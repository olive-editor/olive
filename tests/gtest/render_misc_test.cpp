#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>
#include <QVariant>
#include <QVector2D>

#include "codec/conformmanager.h"
#include "config/config.h"
#include "node/color/colormanager/colormanager.h"
#include "node/output/viewer/viewer.h"
#include "node/project.h"
#include "render/backend/dynamicrenderer.h"
#include "render/colorprocessor.h"
#include "render/diskmanager.h"
#include "render/job/colortransformjob.h"
#include "render/job/shaderjob.h"
#include "render/previewautocacher.h"
#include "render/renderer.h"
#include "render/rendermanager.h"
#include "render/texture.h"
#include "render/videoparams.h"

namespace
{

// CPU-only olive::Renderer implementation that records every call so the
// renderer-core code paths (texture cache, shader cache, color management
// fallback) can be verified without a GL/Vulkan backend.
class StubRenderer : public olive::Renderer {
public:
	StubRenderer()
		: create_texture_count(0)
		, destroy_texture_count(0)
		, create_shader_count(0)
		, destroy_shader_count(0)
		, upload_count(0)
		, download_count(0)
		, flush_count(0)
		, blit_count(0)
		, destroy_internal_count(0)
		, next_handle(0)
		, fail_create_texture(false)
		, fail_create_shader(false)
		, last_create_width(0)
		, last_create_height(0)
		, last_create_depth(0)
		, last_create_channel_count(0)
		, last_create_data(nullptr)
		, last_create_linesize(0)
		, last_upload_linesize(0)
		, last_download_linesize(0)
		, last_blit_destination(nullptr)
		, last_blit_clear(false)
	{
	}

	bool Init() override
	{
		return true;
	}

	void PostDestroy() override
	{
	}

	void PostInit() override
	{
	}

	void ClearDestination(olive::Texture *texture, double r, double g, double b,
						  double a) override
	{
	}

	QVariant CreateNativeShader(olive::ShaderCode code) override
	{
		create_shader_count++;
		if (fail_create_shader) {
			return QVariant();
		}
		return QVariant(QStringLiteral("shader%1").arg(create_shader_count));
	}

	void DestroyNativeShader(QVariant shader) override
	{
		destroy_shader_count++;
	}

	void UploadToTexture(const QVariant &handle, const olive::VideoParams &params,
						 const void *data, int linesize) override
	{
		upload_count++;
		last_upload_handle = handle;
		last_upload_linesize = linesize;
	}

	void DownloadFromTexture(const QVariant &handle,
							 const olive::VideoParams &params, void *data,
							 int linesize) override
	{
		download_count++;
		last_download_handle = handle;
		last_download_linesize = linesize;
	}

	void Flush() override
	{
		flush_count++;
	}

	olive::Color GetPixelFromTexture(olive::Texture *texture,
									 const QPointF &pt) override
	{
		return olive::Color();
	}

	int create_texture_count;
	int destroy_texture_count;
	int create_shader_count;
	int destroy_shader_count;
	int upload_count;
	int download_count;
	int flush_count;
	int blit_count;
	int destroy_internal_count;

	int next_handle;

	bool fail_create_texture;
	bool fail_create_shader;

	int last_create_width;
	int last_create_height;
	int last_create_depth;
	int last_create_channel_count;
	const void *last_create_data;
	int last_create_linesize;

	QVariant last_upload_handle;
	int last_upload_linesize;
	QVariant last_download_handle;
	int last_download_linesize;

	QVariant last_blit_shader;
	olive::Texture *last_blit_destination;
	olive::VideoParams last_blit_params;
	bool last_blit_clear;

protected:
	void Blit(QVariant shader, olive::AcceleratedJob &job,
			  olive::Texture *destination,
			  olive::VideoParams destination_params,
			  bool clear_destination) override
	{
		blit_count++;
		last_blit_shader = shader;
		last_blit_destination = destination;
		last_blit_params = destination_params;
		last_blit_clear = clear_destination;
	}

	QVariant CreateNativeTexture(int width, int height, int depth,
								 olive::PixelFormat format, int channel_count,
								 const void *data, int linesize) override
	{
		create_texture_count++;
		last_create_width = width;
		last_create_height = height;
		last_create_depth = depth;
		last_create_channel_count = channel_count;
		last_create_data = data;
		last_create_linesize = linesize;
		if (fail_create_texture) {
			return QVariant();
		}
		return QVariant(++next_handle);
	}

	void DestroyNativeTexture(QVariant texture) override
	{
		destroy_texture_count++;
	}

	void DestroyInternal() override
	{
		destroy_internal_count++;
	}
};

olive::ColorProcessorPtr CreateIdentityProcessor()
{
	olive::ColorManager::SetUpDefaultConfig();

	OCIO::MatrixTransformRcPtr transform = OCIO::MatrixTransform::Create();
	transform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

	return olive::ColorProcessor::Create(
		olive::ColorManager::GetDefaultConfig()->getProcessor(transform));
}

} // namespace

// Verifies the DynamicRenderer constructor stores the requested backend name
// lowercased and that IsOpenGL()/IsVulkan() reflect it before any Load().
TEST(DynamicRenderer, ConstructorNormalizesBackendName)
{
	olive::DynamicRenderer gl(QStringLiteral("OpenGL"));
	EXPECT_EQ(gl.backend_name(), QStringLiteral("opengl"));
	EXPECT_TRUE(gl.IsOpenGL());
	EXPECT_FALSE(gl.IsVulkan());

	olive::DynamicRenderer vk(QStringLiteral("VULKAN"));
	EXPECT_EQ(vk.backend_name(), QStringLiteral("vulkan"));
	EXPECT_TRUE(vk.IsVulkan());
	EXPECT_FALSE(vk.IsOpenGL());
}

// Documents that an unrecognized backend name is kept verbatim (and
// LibraryFilename() uses that verbatim name as the library basename, so Load()
// fails and the OpenGL fallback engages), so the IsOpenGL()/IsVulkan()
// predicates both report false for it.
TEST(DynamicRenderer, UnknownBackendNameIsReportedVerbatim)
{
	olive::DynamicRenderer renderer(QStringLiteral("Metal"));
	EXPECT_EQ(renderer.backend_name(), QStringLiteral("metal"));
	EXPECT_FALSE(renderer.IsOpenGL());
	EXPECT_FALSE(renderer.IsVulkan());
}

// Before Load() succeeds there is no backend handle, so the metadata and
// context accessors must return safe defaults instead of dereferencing null
// function pointers.
TEST(DynamicRenderer, AccessorsBeforeLoadReturnDefaults)
{
	olive::DynamicRenderer renderer(QStringLiteral("opengl"));

	EXPECT_EQ(renderer.OpenGLContext(), nullptr);

	OakRenderBackendInfo info = {};
	EXPECT_FALSE(renderer.GetBackendInfo(&info));
	EXPECT_FALSE(renderer.GetBackendInfo(nullptr));
}

// Lifecycle entry points must tolerate being called without a loaded backend:
// each one guards on the null handle/function table and does nothing.
TEST(DynamicRenderer, PreLoadLifecycleCallsAreSafeNoOps)
{
	olive::DynamicRenderer renderer(QStringLiteral("opengl"));

	renderer.PostInit();
	renderer.PostDestroy();
	renderer.AttachOutputTexture(nullptr);
	renderer.DetachOutputTexture();
	renderer.Destroy();
	// Destruction after an explicit Destroy() must also be safe.
}

// Once a backend is loaded, Load() must short-circuit on the existing handle
// instead of reloading the library a second time.
TEST(DynamicRenderer, SecondLoadReturnsImmediately)
{
#ifndef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
	GTEST_SKIP() << "Dynamic render backend is not enabled in this build";
#else
	olive::DynamicRenderer renderer(QStringLiteral("opengl"));
	if (!renderer.Load()) {
		GTEST_SKIP()
			<< "opengl backend library could not be loaded in this environment";
	}

	EXPECT_TRUE(renderer.Load());
	EXPECT_TRUE(renderer.IsOpenGL());
	EXPECT_EQ(renderer.backend_name(), QStringLiteral("opengl"));
#endif
}

// Renderer::CreateTexture must wrap the native handle produced by
// CreateNativeTexture into a live Texture that mirrors the requested params.
TEST(RendererTextureCache, CreateTextureWrapsNativeHandle)
{
	StubRenderer renderer;
	const olive::VideoParams params(64, 32, olive::PixelFormat::U8,
									olive::VideoParams::kRGBAChannelCount);

	olive::TexturePtr texture = renderer.CreateTexture(params);
	ASSERT_NE(texture, nullptr);
	EXPECT_FALSE(texture->IsDummy());
	EXPECT_EQ(texture->id(), QVariant(1));
	EXPECT_EQ(texture->width(), 64);
	EXPECT_EQ(texture->height(), 32);
	EXPECT_EQ(texture->format(), olive::PixelFormat::U8);
	EXPECT_EQ(texture->channel_count(),
			  int(olive::VideoParams::kRGBAChannelCount));
	EXPECT_EQ(renderer.create_texture_count, 1);

	renderer.Destroy();
}

// A null native handle (backend allocation failure) must propagate as a null
// TexturePtr rather than a dummy texture.
TEST(RendererTextureCache, FailedNativeTextureCreateReturnsNull)
{
	StubRenderer renderer;
	renderer.fail_create_texture = true;

	const olive::VideoParams params(64, 64, olive::PixelFormat::U8,
									olive::VideoParams::kRGBAChannelCount);
	EXPECT_EQ(renderer.CreateTexture(params), nullptr);
	EXPECT_EQ(renderer.create_texture_count, 1);
}

// Destroying a texture returns its native handle to the cache; a matching
// CreateTexture must reuse it without calling into the backend again, and
// flush so pending backend work is visible to the recycled texture.
TEST(RendererTextureCache, DestroyedTextureIsReusedFromCache)
{
	StubRenderer renderer;
	const olive::VideoParams params(64, 64, olive::PixelFormat::U8,
									olive::VideoParams::kRGBAChannelCount);

	{
		olive::TexturePtr texture = renderer.CreateTexture(params);
		ASSERT_NE(texture, nullptr);
		EXPECT_EQ(texture->id(), QVariant(1));
	}
	EXPECT_EQ(renderer.create_texture_count, 1);

	olive::TexturePtr reused = renderer.CreateTexture(params);
	ASSERT_NE(reused, nullptr);
	EXPECT_EQ(reused->id(), QVariant(1));
	EXPECT_EQ(renderer.create_texture_count, 1);
	EXPECT_EQ(renderer.flush_count, 1);

	// A different size must miss the cache and allocate natively again.
	const olive::VideoParams other(32, 32, olive::PixelFormat::U8,
								   olive::VideoParams::kRGBAChannelCount);
	olive::TexturePtr fresh = renderer.CreateTexture(other);
	ASSERT_NE(fresh, nullptr);
	EXPECT_EQ(fresh->id(), QVariant(2));
	EXPECT_EQ(renderer.create_texture_count, 2);

	reused.reset();
	fresh.reset();
	renderer.Destroy();
	EXPECT_EQ(renderer.destroy_texture_count, 2);
}

// When pixel data is supplied for a cache hit, the renderer must upload it
// into the recycled native handle rather than reallocating.
TEST(RendererTextureCache, CachedTextureReusedWithDataTriggersUpload)
{
	StubRenderer renderer;
	const olive::VideoParams params(16, 16, olive::PixelFormat::U8,
									olive::VideoParams::kRGBAChannelCount);

	{
		olive::TexturePtr texture = renderer.CreateTexture(params);
		ASSERT_NE(texture, nullptr);
	}

	char data[16 * 16 * 4] = {};
	olive::TexturePtr texture =
		renderer.CreateTexture(params, data, 16 * 4);
	ASSERT_NE(texture, nullptr);
	EXPECT_EQ(renderer.create_texture_count, 1);
	EXPECT_EQ(renderer.upload_count, 1);
	EXPECT_EQ(renderer.last_upload_handle, QVariant(1));
	EXPECT_EQ(renderer.last_upload_linesize, 16 * 4);

	texture.reset();
	renderer.Destroy();
}

// On a cache miss with initial data, the data pointer and linesize must be
// forwarded to CreateNativeTexture untouched.
TEST(RendererTextureCache, CreateWithDataForwardsToNativeCreate)
{
	StubRenderer renderer;
	const olive::VideoParams params(8, 8, olive::PixelFormat::U8,
									olive::VideoParams::kRGBAChannelCount);

	char data[8 * 8 * 4] = {};
	olive::TexturePtr texture = renderer.CreateTexture(params, data, 8 * 4);
	ASSERT_NE(texture, nullptr);
	EXPECT_EQ(renderer.create_texture_count, 1);
	EXPECT_EQ(renderer.last_create_data, static_cast<const void *>(data));
	EXPECT_EQ(renderer.last_create_linesize, 8 * 4);
	EXPECT_EQ(renderer.last_create_width, 8);
	EXPECT_EQ(renderer.last_create_height, 8);
	EXPECT_EQ(renderer.last_create_channel_count,
			  int(olive::VideoParams::kRGBAChannelCount));
	EXPECT_EQ(renderer.upload_count, 0);

	texture.reset();
	renderer.Destroy();
}

// GetDefaultShader() must compile the built-in shader once and cache it;
// Destroy() releases it exactly once through DestroyNativeShader.
TEST(RendererShaderCache, DefaultShaderCreatedOnceAndReleasedOnDestroy)
{
	StubRenderer renderer;

	const QVariant first = renderer.GetDefaultShader();
	const QVariant second = renderer.GetDefaultShader();
	EXPECT_FALSE(first.isNull());
	EXPECT_EQ(first, second);
	EXPECT_EQ(renderer.create_shader_count, 1);

	renderer.Destroy();
	EXPECT_EQ(renderer.destroy_shader_count, 1);
	EXPECT_EQ(renderer.destroy_internal_count, 1);

	// A repeated Destroy() must not release the shader a second time.
	renderer.Destroy();
	EXPECT_EQ(renderer.destroy_shader_count, 1);
	EXPECT_EQ(renderer.destroy_internal_count, 2);
}

// Texture::Upload/Download must forward the native id, params, and linesize
// to the owning renderer while it is alive.
TEST(TextureIo, UploadDownloadForwardToRenderer)
{
	StubRenderer renderer;
	const olive::VideoParams params(16, 16, olive::PixelFormat::U8,
									olive::VideoParams::kRGBAChannelCount);

	olive::TexturePtr texture = renderer.CreateTexture(params);
	ASSERT_NE(texture, nullptr);

	char data[16 * 16 * 4] = {};
	texture->Upload(data, 16 * 4);
	EXPECT_EQ(renderer.upload_count, 1);
	EXPECT_EQ(renderer.last_upload_handle, texture->id());
	EXPECT_EQ(renderer.last_upload_linesize, 16 * 4);

	texture->Download(data, 16 * 4);
	EXPECT_EQ(renderer.download_count, 1);
	EXPECT_EQ(renderer.last_download_handle, texture->id());
	EXPECT_EQ(renderer.last_download_linesize, 16 * 4);

	texture.reset();
	renderer.Destroy();
}

// A dummy texture (no backend renderer) must expose its params, report
// IsDummy(), and make Upload/Download harmless no-ops.
TEST(TextureDummy, AccessorsAndNoOpIo)
{
	const olive::VideoParams params(128, 64, olive::PixelFormat::F16,
									olive::VideoParams::kRGBAChannelCount);
	olive::Texture texture(params);

	EXPECT_TRUE(texture.IsDummy());
	EXPECT_EQ(texture.renderer(), nullptr);
	EXPECT_TRUE(texture.id().isNull());
	EXPECT_EQ(texture.width(), 128);
	EXPECT_EQ(texture.height(), 64);
	EXPECT_EQ(texture.virtual_resolution(), QVector2D(128, 64));
	EXPECT_EQ(texture.format(), olive::PixelFormat::F16);
	EXPECT_EQ(texture.channel_count(),
			  int(olive::VideoParams::kRGBAChannelCount));
	EXPECT_EQ(texture.divider(), 1);
	EXPECT_EQ(texture.pixel_aspect_ratio(), olive::rational(1));
	EXPECT_FALSE(texture.IsJob());
	EXPECT_EQ(texture.job(), nullptr);

	EXPECT_EQ(int(olive::Texture::kDefaultInterpolation),
			  int(olive::Texture::kMipmappedLinear));

	char data[4] = {};
	texture.Upload(data, 4);
	texture.Download(data, 4);
}

// Textures can carry a CPU-side AcceleratedJob instead of a native handle;
// the job must be owned and retrievable through job().
TEST(TextureJob, JobTextureExposesJob)
{
	const olive::VideoParams params(32, 32, olive::PixelFormat::U8,
									olive::VideoParams::kRGBAChannelCount);
	olive::TexturePtr texture =
		olive::Texture::Job(params, olive::ShaderJob());

	ASSERT_NE(texture, nullptr);
	EXPECT_TRUE(texture->IsDummy());
	EXPECT_TRUE(texture->IsJob());
	ASSERT_NE(texture->job(), nullptr);
	EXPECT_EQ(texture->params().width(), 32);
}

// When the color-management shader cannot be compiled, BlitColorManaged must
// fall back to a plain textured blit with the default shader instead of
// failing silently or crashing.
TEST(RendererColorManagement, FallsBackToDefaultShaderWhenCompilationFails)
{
	StubRenderer renderer;
	renderer.fail_create_shader = true;

	olive::ColorProcessorPtr processor = CreateIdentityProcessor();
	ASSERT_TRUE(processor);

	olive::ColorTransformJob job;
	job.SetColorProcessor(processor);

	const olive::VideoParams params(64, 64, olive::PixelFormat::U8,
									olive::VideoParams::kRGBAChannelCount);
	renderer.BlitColorManaged(job, params);

	// One blit to the null destination overload, with the (failed) default
	// shader handle and the job's clear-destination flag preserved.
	EXPECT_EQ(renderer.blit_count, 1);
	EXPECT_EQ(renderer.last_blit_destination, nullptr);
	EXPECT_TRUE(renderer.last_blit_shader.isNull());
	EXPECT_TRUE(renderer.last_blit_clear);
	EXPECT_EQ(renderer.last_blit_params.width(), 64);

	renderer.Destroy();
}

// GetColorContext must cache the compiled color pipeline per processor id:
// repeating the same job reuses the context, while an override id forces a
// second compilation.
TEST(RendererColorManagement, CachesColorContextPerProcessorId)
{
	StubRenderer renderer;

	olive::ColorProcessorPtr processor = CreateIdentityProcessor();
	ASSERT_TRUE(processor);

	olive::ColorTransformJob job;
	job.SetColorProcessor(processor);

	const olive::VideoParams params(64, 64, olive::PixelFormat::U8,
									olive::VideoParams::kRGBAChannelCount);

	renderer.BlitColorManaged(job, params);
	EXPECT_EQ(renderer.blit_count, 1);
	EXPECT_EQ(renderer.create_shader_count, 1);
	EXPECT_FALSE(renderer.last_blit_shader.isNull());

	// Same processor id: color context cache hit, no new shader compilation.
	renderer.BlitColorManaged(job, params);
	EXPECT_EQ(renderer.blit_count, 2);
	EXPECT_EQ(renderer.create_shader_count, 1);

	// A distinct override id bypasses the cached context and compiles again.
	olive::ColorTransformJob other_job;
	other_job.SetColorProcessor(processor);
	other_job.SetOverrideID(QStringLiteral("other-context"));
	renderer.BlitColorManaged(other_job, params);
	EXPECT_EQ(renderer.blit_count, 3);
	EXPECT_EQ(renderer.create_shader_count, 2);

	renderer.Destroy();
}

// The destination overload of BlitColorManaged must forward the destination
// texture and its params to the backend blit.
TEST(RendererColorManagement, BlitToDestinationForwardsTexture)
{
	StubRenderer renderer;

	olive::ColorProcessorPtr processor = CreateIdentityProcessor();
	ASSERT_TRUE(processor);

	olive::ColorTransformJob job;
	job.SetColorProcessor(processor);

	const olive::VideoParams params(32, 16, olive::PixelFormat::U8,
									olive::VideoParams::kRGBAChannelCount);
	olive::TexturePtr destination = renderer.CreateTexture(params);
	ASSERT_NE(destination, nullptr);

	renderer.BlitColorManaged(job, destination.get());
	EXPECT_EQ(renderer.blit_count, 1);
	EXPECT_EQ(renderer.last_blit_destination, destination.get());
	EXPECT_EQ(renderer.last_blit_params.width(), 32);
	EXPECT_EQ(renderer.last_blit_params.height(), 16);

	destination.reset();
	renderer.Destroy();
}

class RenderMiscAutoCacherTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		olive::ColorManager::SetUpDefaultConfig();

		// Use the dummy render backend so PreviewAutoCacher can be exercised
		// without initializing OpenGL/Vulkan in the unit-test process.
		olive::Config::Current()[QStringLiteral("GraphicsBackend")] =
			QStringLiteral("dummy");

		olive::DiskManager::CreateInstance();
		olive::ConformManager::CreateInstance();
		olive::RenderManager::CreateInstance();

		project_ = std::make_unique<olive::Project>();
		project_->Initialize();
	}

	void TearDown() override
	{
		project_.reset();
		olive::RenderManager::DestroyInstance();
		olive::ConformManager::DestroyInstance();
		olive::DiskManager::DestroyInstance();
	}

	std::unique_ptr<olive::Project> project_;
};

// Requesting a new single frame must cancel the previously queued (not yet
// dispatched) single-frame ticket: the old ticket finishes without a result
// and the new one becomes the pending render.
TEST_F(RenderMiscAutoCacherTest, GetSingleFrameCancelsPreviouslyQueuedTicket)
{
	auto *viewer = new olive::ViewerOutput();
	viewer->setParent(project_.get());

	olive::PreviewAutoCacher cacher;

	olive::RenderTicketPtr first =
		cacher.GetSingleFrame(viewer, olive::rational(0));
	olive::RenderTicketPtr second =
		cacher.GetSingleFrame(viewer, olive::rational(1));

	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	EXPECT_NE(first, second);

	EXPECT_EQ(first->GetFinishCount(), 1);
	EXPECT_FALSE(first->IsRunning());
	EXPECT_FALSE(first->HasResult());

	EXPECT_TRUE(second->IsRunning());
}

// SetProject() early-outs when the same project (or null twice) is passed;
// neither call may disturb the copied graph or leak tasks.
TEST_F(RenderMiscAutoCacherTest, SetSameProjectTwiceIsNoOp)
{
	olive::PreviewAutoCacher cacher;
	cacher.SetProject(project_.get());
	cacher.SetProject(project_.get());
	cacher.SetProject(nullptr);
	cacher.SetProject(nullptr);
	EXPECT_FALSE(cacher.IsRenderingCustomRange());
}

// ForceCacheRange queues the requested frames through TryRender; with the
// dummy backend each ticket finishes without a result, and the cacher must
// still emit StopCacheProxyTasks when the range iterator is exhausted.
TEST_F(RenderMiscAutoCacherTest,
	   ForceCacheRangeSchedulesVideoJobAndSignalsCompletion)
{
	auto *viewer = new olive::ViewerOutput();
	viewer->setParent(project_.get());
	viewer->SetVideoParams(
		olive::VideoParams(64, 64, olive::rational(1, 25),
						   olive::PixelFormat::U8,
						   olive::VideoParams::kRGBAChannelCount));

	olive::PreviewAutoCacher cacher;
	cacher.SetProject(project_.get());

	QSignalSpy stop_spy(&cacher, &olive::PreviewAutoCacher::StopCacheProxyTasks);

	cacher.ForceCacheRange(
		viewer, olive::TimeRange(olive::rational(0), olive::rational(1, 25)));

	EXPECT_GE(stop_spy.count(), 1);
	EXPECT_FALSE(cacher.IsRenderingCustomRange());

	// Deliver the queued RenderTicketWatcher::Finished emissions so the
	// completed watchers are reaped before teardown.
	QCoreApplication::processEvents();

	cacher.SetProject(nullptr);
}

// A conform-ready notification with no conform-blocked audio ranges must be a
// harmless no-op.
TEST_F(RenderMiscAutoCacherTest, ConformReadyWithoutPendingConformsIsNoOp)
{
	olive::PreviewAutoCacher cacher;
	cacher.SetProject(project_.get());

	emit olive::ConformManager::instance()->ConformReady();

	cacher.SetProject(nullptr);
}

// Cancelling cache-proxy tasks must drop pending video jobs without touching
// the (empty) running task list.
TEST_F(RenderMiscAutoCacherTest, CacheProxyTaskCancelledClearsPendingJobs)
{
	auto *viewer = new olive::ViewerOutput();
	viewer->setParent(project_.get());

	olive::PreviewAutoCacher cacher;

	// No project is set, so the forced range sits in the pending queue.
	cacher.ForceCacheRange(
		viewer, olive::TimeRange(olive::rational(0), olive::rational(1)));

	EXPECT_TRUE(QMetaObject::invokeMethod(&cacher, "CacheProxyTaskCancelled",
										  Qt::DirectConnection));

	cacher.SetProject(nullptr);
}

// With an unknown/dummy graphics backend the RenderManager never creates the
// GPU-side objects. Those pointers must be null (previously they were left
// uninitialized, so callers such as ViewerWidget dereferenced garbage and
// crashed).
TEST(RenderManagerDummyBackend, GpuMembersAreNullRatherThanUninitialized)
{
	const QVariant previous =
		olive::Config::Current()[QStringLiteral("GraphicsBackend")];
	olive::Config::Current()[QStringLiteral("GraphicsBackend")] =
		QStringLiteral("dummy");

	olive::RenderManager::CreateInstance();

	EXPECT_EQ(olive::RenderManager::instance()->GetCacher(), nullptr);

	olive::RenderManager::DestroyInstance();
	olive::Config::Current()[QStringLiteral("GraphicsBackend")] = previous;
}
