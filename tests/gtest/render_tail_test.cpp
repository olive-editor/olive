#include <gtest/gtest.h>

#include <cstring>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QVariant>

#include "codec/conformmanager.h"
#include "config/config.h"
#include "core.h"
#include "node/color/colormanager/colormanager.h"
#include "node/generator/solid/solid.h"
#include "node/keying/chromakey/chromakey.h"
#include "node/output/viewer/viewer.h"
#include "node/project.h"
#include "olive/core/render/audioparams.h"
#include "olive/core/render/samplebuffer.h"
#include "render/audioplaybackcache.h"
#include "render/backend/dynamicrenderer.h"
#include "render/colorprocessor.h"
#include "render/diskmanager.h"
#include "render/job/colortransformjob.h"
#include "render/previewautocacher.h"
#include "render/renderer.h"
#include "render/rendermanager.h"
#include "render/texture.h"
#include "render/videoparams.h"

namespace
{

// CPU-only olive::Renderer that records the shader code it is asked to compile
// so Renderer::GetColorContext() shader generation can be verified without a
// GL/Vulkan backend.
class ShaderCaptureRenderer : public olive::Renderer {
public:
	ShaderCaptureRenderer()
		: create_shader_count(0)
		, create_texture_count(0)
		, blit_count(0)
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
		last_frag_code = code.frag_code();
		last_vert_code = code.vert_code();
		return QVariant(create_shader_count);
	}

	void DestroyNativeShader(QVariant shader) override
	{
	}

	void UploadToTexture(const QVariant &handle, const olive::VideoParams &params,
						 const void *data, int linesize) override
	{
	}

	void DownloadFromTexture(const QVariant &handle,
							 const olive::VideoParams &params, void *data,
							 int linesize) override
	{
	}

	void Flush() override
	{
	}

	olive::Color GetPixelFromTexture(olive::Texture *texture,
									 const QPointF &pt) override
	{
		return olive::Color();
	}

	int create_shader_count;
	int create_texture_count;
	int blit_count;
	QString last_frag_code;
	QString last_vert_code;

protected:
	void Blit(QVariant shader, olive::AcceleratedJob &job,
			  olive::Texture *destination, olive::VideoParams destination_params,
			  bool clear_destination) override
	{
		blit_count++;
	}

	QVariant CreateNativeTexture(int width, int height, int depth,
								 olive::PixelFormat format, int channel_count,
								 const void *data, int linesize) override
	{
		create_texture_count++;
		return QVariant(create_texture_count);
	}

	void DestroyNativeTexture(QVariant texture) override
	{
	}

	void DestroyInternal() override
	{
	}
};

olive::ColorProcessorPtr MakeIdentityProcessor()
{
	olive::ColorManager::SetUpDefaultConfig();

	OCIO::MatrixTransformRcPtr transform = OCIO::MatrixTransform::Create();
	transform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

	return olive::ColorProcessor::Create(
		olive::ColorManager::GetDefaultConfig()->getProcessor(transform));
}

bool WriteFile(const QString &path, qint64 size)
{
	QFile file(path);
	if (!file.open(QFile::WriteOnly)) {
		return false;
	}
	file.write(QByteArray(static_cast<int>(size), 'x'));
	file.close();
	return true;
}

bool ReadBytesAt(const QString &path, qint64 offset, qint64 len, QByteArray *out)
{
	QFile f(path);
	if (!f.open(QFile::ReadOnly)) {
		return false;
	}
	if (!f.seek(offset)) {
		return false;
	}
	*out = f.read(len);
	return out->size() == len;
}

float BytesToFloat(const QByteArray &bytes)
{
	float v;
	memcpy(&v, bytes.constData(), sizeof(v));
	return v;
}

// AudioPlaybackCache always stores audio in fixed-size segments of 10 MB per
// channel (AudioPlaybackCache::kDefaultSegmentSizePerChannel).
const qint64 kSegmentSize = 10 * 1024 * 1024;

} // namespace

// A DynamicRenderer constructed with an empty backend name must report no
// backend type; the name is stored verbatim (only lowercased).
TEST(DynamicRenderer, EmptyBackendNameHasNoBackendType)
{
	olive::DynamicRenderer renderer{ QString() };
	EXPECT_TRUE(renderer.backend_name().isEmpty());
	EXPECT_FALSE(renderer.IsOpenGL());
	EXPECT_FALSE(renderer.IsVulkan());

	// Without a loaded backend, context and info accessors stay at defaults
	EXPECT_EQ(renderer.OpenGLContext(), nullptr);

	OakRenderBackendInfo info = {};
	EXPECT_FALSE(renderer.GetBackendInfo(&info));
}

// BackendFromString lowercases its input before comparing, so mixed-case
// spellings of every backend must resolve correctly.
TEST(RenderManagerBackendStrings, FromStringIsCaseInsensitive)
{
	EXPECT_EQ(olive::RenderManager::BackendFromString(QStringLiteral("VULKAN")),
			  olive::RenderManager::kVulkan);
	EXPECT_EQ(
		olive::RenderManager::BackendFromString(QStringLiteral("MultiProcess")),
		olive::RenderManager::kMultiProcess);
	EXPECT_EQ(olive::RenderManager::BackendFromString(QStringLiteral("DUMMY")),
			  olive::RenderManager::kDummy);

	// Unknown and empty strings fall through to OpenGL
	EXPECT_EQ(olive::RenderManager::BackendFromString(QString()),
			  olive::RenderManager::kOpenGL);
}

// BackendToString has a default return after the switch for out-of-range enum
// values, which must be the OpenGL string.
TEST(RenderManagerBackendStrings, ToStringFallsBackToOpenGLForUnknownEnum)
{
	EXPECT_EQ(olive::RenderManager::BackendToString(
				  static_cast<olive::RenderManager::Backend>(42)),
			  QStringLiteral("opengl"));
}

// A ColorTransformJob with a custom OCIO function name must have that name
// embedded in the shader generated by GetColorContext (it is passed to
// GpuShaderDesc::setFunctionName).
TEST(RendererColorContext, CustomFunctionNameIsCompiledIntoShader)
{
	ShaderCaptureRenderer renderer;

	olive::ColorProcessorPtr processor = MakeIdentityProcessor();
	ASSERT_TRUE(processor);

	olive::ColorTransformJob job;
	job.SetColorProcessor(processor);
	job.SetFunctionName(QStringLiteral("MyCustomOcioFunc"));

	const olive::VideoParams params(32, 32, olive::PixelFormat::U8,
									olive::VideoParams::kRGBAChannelCount);
	renderer.BlitColorManaged(job, params);

	ASSERT_EQ(renderer.create_shader_count, 1);
	EXPECT_TRUE(
		renderer.last_frag_code.contains(QStringLiteral("MyCustomOcioFunc")));
	EXPECT_EQ(renderer.blit_count, 1);

	renderer.Destroy();
}

// When the job names a custom shader source node, GetColorContext must ask the
// node for its shader code (with the OCIO stub) instead of using the built-in
// colormanage stub. ChromaKeyNode wraps the stub in its chroma-key shader.
TEST(RendererColorContext, CustomShaderSourceSuppliesFragmentCode)
{
	ShaderCaptureRenderer renderer;

	olive::ColorProcessorPtr processor = MakeIdentityProcessor();
	ASSERT_TRUE(processor);

	olive::ChromaKeyNode key_node;

	olive::ColorTransformJob job;
	job.SetColorProcessor(processor);
	job.SetNeedsCustomShader(&key_node);

	const olive::VideoParams params(32, 32, olive::PixelFormat::U8,
									olive::VideoParams::kRGBAChannelCount);
	renderer.BlitColorManaged(job, params);

	ASSERT_EQ(renderer.create_shader_count, 1);
	// A uniform name unique to chromakey.frag proves the node's code was used
	EXPECT_TRUE(renderer.last_frag_code.contains(QStringLiteral("color_key")));
	EXPECT_EQ(renderer.blit_count, 1);

	renderer.Destroy();
}

class RenderTailAutoCacherTest : public ::testing::Test {
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

	olive::ViewerOutput *CreateViewerWithParams()
	{
		auto *viewer = new olive::ViewerOutput();
		viewer->setParent(project_.get());
		viewer->SetVideoParams(
			olive::VideoParams(64, 64, olive::rational(1, 25),
							   olive::PixelFormat::U8,
							   olive::VideoParams::kRGBAChannelCount));
		return viewer;
	}

	std::unique_ptr<olive::Project> project_;
};

// While renders are paused, a forced cache range must sit in the pending queue;
// unpausing dispatches it and emits StopCacheProxyTasks once the (single frame)
// range iterator is exhausted.
TEST_F(RenderTailAutoCacherTest, PausedRendersDelayForcedCacheRange)
{
	olive::ViewerOutput *viewer = CreateViewerWithParams();

	olive::PreviewAutoCacher cacher;
	cacher.SetProject(project_.get());

	QSignalSpy stop_spy(&cacher, &olive::PreviewAutoCacher::StopCacheProxyTasks);

	cacher.SetRendersPaused(true);
	cacher.ForceCacheRange(
		viewer, olive::TimeRange(olive::rational(0), olive::rational(1, 25)));
	EXPECT_EQ(stop_spy.count(), 0);

	cacher.SetRendersPaused(false);
	EXPECT_GE(stop_spy.count(), 1);

	// Deliver the queued RenderTicketWatcher::Finished emissions so the
	// completed watchers are reaped before teardown.
	QCoreApplication::processEvents();

	cacher.SetProject(nullptr);
}

// The thumbnail pause gates only the video-job half of TryRender, so a forced
// cache range queued while thumbnails are paused must wait for the unpause.
TEST_F(RenderTailAutoCacherTest, PausedThumbnailsDelayForcedCacheRange)
{
	olive::ViewerOutput *viewer = CreateViewerWithParams();

	olive::PreviewAutoCacher cacher;
	cacher.SetProject(project_.get());

	QSignalSpy stop_spy(&cacher, &olive::PreviewAutoCacher::StopCacheProxyTasks);

	cacher.SetThumbnailsPaused(true);
	cacher.ForceCacheRange(
		viewer, olive::TimeRange(olive::rational(0), olive::rational(1, 25)));
	EXPECT_EQ(stop_spy.count(), 0);

	cacher.SetThumbnailsPaused(false);
	EXPECT_GE(stop_spy.count(), 1);

	QCoreApplication::processEvents();

	cacher.SetProject(nullptr);
}

// With a project set, GetSingleFrame resolves the node through the ProjectCopier
// and dispatches a real render ticket. With the dummy backend the underlying
// ticket finishes without a result, and the passthrough ticket must be finished
// once the watcher signals completion.
TEST_F(RenderTailAutoCacherTest, GetSingleFrameDispatchesThroughProjectCopy)
{
	olive::ViewerOutput *viewer = CreateViewerWithParams();

	auto *solid = new olive::SolidGenerator();
	solid->setParent(project_.get());

	olive::PreviewAutoCacher cacher;
	cacher.SetProject(project_.get());

	olive::RenderTicketPtr ticket =
		cacher.GetSingleFrame(solid, viewer, olive::rational(0));
	ASSERT_NE(ticket, nullptr);
	EXPECT_TRUE(ticket->IsRunning());

	// The dummy backend has no render threads, so the dispatched ticket can
	// only be finished through the clear path (covered in detail by the
	// ClearSingleFrameRenders tests below).
	cacher.ClearSingleFrameRenders();

	EXPECT_EQ(ticket->GetFinishCount(), 1);
	EXPECT_FALSE(ticket->IsRunning());
	EXPECT_FALSE(ticket->HasResult());

	cacher.SetProject(nullptr);
}

// ClearSingleFrameRenders must cancel every dispatched (but no longer running)
// single-frame passthrough: the ticket is finished without a result and the
// watcher is reaped synchronously through VideoRendered.
TEST_F(RenderTailAutoCacherTest, ClearSingleFrameRendersFinishesDispatchedTicket)
{
	olive::ViewerOutput *viewer = CreateViewerWithParams();

	auto *solid = new olive::SolidGenerator();
	solid->setParent(project_.get());

	olive::PreviewAutoCacher cacher;
	cacher.SetProject(project_.get());

	olive::RenderTicketPtr ticket =
		cacher.GetSingleFrame(solid, viewer, olive::rational(0));
	ASSERT_NE(ticket, nullptr);
	ASSERT_TRUE(ticket->IsRunning());

	cacher.ClearSingleFrameRenders();

	EXPECT_EQ(ticket->GetFinishCount(), 1);
	EXPECT_FALSE(ticket->IsRunning());
	EXPECT_FALSE(ticket->HasResult());

	// Flush the stale queued watcher notification (its receiver is gone now).
	QCoreApplication::processEvents();

	cacher.SetProject(nullptr);
}

// ClearSingleFrameRendersThatArentRunning follows the same path for the dummy
// backend, whose tickets are never running by the time they can be cleared.
TEST_F(RenderTailAutoCacherTest,
	   ClearSingleFrameRendersThatArentRunningFinishesDispatchedTicket)
{
	olive::ViewerOutput *viewer = CreateViewerWithParams();

	auto *solid = new olive::SolidGenerator();
	solid->setParent(project_.get());

	olive::PreviewAutoCacher cacher;
	cacher.SetProject(project_.get());

	olive::RenderTicketPtr ticket =
		cacher.GetSingleFrame(solid, viewer, olive::rational(0));
	ASSERT_NE(ticket, nullptr);
	ASSERT_TRUE(ticket->IsRunning());

	cacher.ClearSingleFrameRendersThatArentRunning();

	EXPECT_EQ(ticket->GetFinishCount(), 1);
	EXPECT_FALSE(ticket->IsRunning());
	EXPECT_FALSE(ticket->HasResult());

	QCoreApplication::processEvents();

	cacher.SetProject(nullptr);
}

class RenderTailDiskCacheTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		if (!temp_dir_.isValid()) {
			GTEST_FAIL() << "Failed to create temporary directory";
		}

		if (!olive::Core::instance()) {
			// Leaked intentionally: matches render_diskcache_test, Core is
			// process-wide and eviction paths call Core::WarnCacheFull().
			new olive::Core(olive::Core::CoreParams());
		}

		olive::DiskManager::CreateInstance();
	}

	void TearDown() override
	{
		olive::DiskManager::DestroyInstance();
	}

	QString MakeSubDir(const QString &name) const
	{
		QDir root(temp_dir_.path());
		if (!root.mkpath(name)) {
			return QString();
		}
		return root.filePath(name);
	}

	QTemporaryDir temp_dir_;
};

// Moving a folder to a new path must broadcast DeletedFrame for every tracked
// file (without deleting anything on disk) and reset the folder state to
// defaults before loading the new path's index.
TEST_F(RenderTailDiskCacheTest, SetPathEmitsDeletedFramesAndResetsState)
{
	const QString sub1 = MakeSubDir(QStringLiteral("move_from"));
	const QString sub2 = MakeSubDir(QStringLiteral("move_to"));
	ASSERT_FALSE(sub1.isEmpty());
	ASSERT_FALSE(sub2.isEmpty());

	olive::DiskCacheFolder folder(sub1);
	folder.SetLimit(12345);

	const QString fn = QDir(sub1).filePath(QStringLiteral("frame"));
	ASSERT_TRUE(WriteFile(fn, 64));
	folder.CreatedFile(fn);

	QSignalSpy spy(&folder, &olive::DiskCacheFolder::DeletedFrame);

	folder.SetPath(sub2);

	EXPECT_EQ(folder.GetPath(), sub2);
	EXPECT_EQ(folder.GetLimit(), 21474836480LL); // back to the 20 GB default
	EXPECT_FALSE(folder.GetClearOnClose());

	ASSERT_EQ(spy.count(), 1);
	const QList<QVariant> args = spy.takeFirst();
	EXPECT_EQ(args.at(0).toString(), sub1);
	EXPECT_EQ(args.at(1).toString(), fn);

	// The file itself is untouched, but it is no longer tracked
	EXPECT_TRUE(QFileInfo::exists(fn));
	EXPECT_FALSE(folder.DeleteSpecificFile(fn));
}

// When the persisted index references files that have since been deleted
// externally, those entries must be skipped on load while surviving files are
// still picked up.
TEST_F(RenderTailDiskCacheTest, PersistedIndexSkipsFilesThatNoLongerExist)
{
	const QString sub = MakeSubDir(QStringLiteral("index_skip"));
	ASSERT_FALSE(sub.isEmpty());

	const QString keep = QDir(sub).filePath(QStringLiteral("keep"));
	const QString gone = QDir(sub).filePath(QStringLiteral("gone"));
	ASSERT_TRUE(WriteFile(keep, 32));
	ASSERT_TRUE(WriteFile(gone, 32));

	{
		olive::DiskCacheFolder folder(sub);
		folder.CreatedFile(keep);
		folder.CreatedFile(gone);
		// Destruction writes the index file into the cache folder
	}

	ASSERT_TRUE(QFile::remove(gone));

	{
		olive::DiskCacheFolder reopened(sub);

		// The missing file was not re-registered, the surviving one was
		EXPECT_TRUE(reopened.DeleteSpecificFile(keep));
		EXPECT_FALSE(reopened.DeleteSpecificFile(gone));
		EXPECT_FALSE(QFileInfo::exists(keep));
	}
}

// The clear-on-close flag is serialized into the index along with the limit,
// so a folder reopened after closing with the flag set must restore it.
TEST_F(RenderTailDiskCacheTest, ClearOnCloseFlagPersistsAcrossInstances)
{
	const QString sub = MakeSubDir(QStringLiteral("persist_clear_flag"));
	ASSERT_FALSE(sub.isEmpty());

	const QString fn = QDir(sub).filePath(QStringLiteral("frame"));
	ASSERT_TRUE(WriteFile(fn, 32));

	{
		olive::DiskCacheFolder folder(sub);
		folder.SetClearOnClose(true);
		folder.CreatedFile(fn);
		// Destruction clears the cache and saves the flag into the index
	}

	ASSERT_FALSE(QFileInfo::exists(fn));

	{
		olive::DiskCacheFolder reopened(sub);
		EXPECT_TRUE(reopened.GetClearOnClose());

		// The cleared entry must not come back through the index either
		EXPECT_FALSE(reopened.DeleteSpecificFile(fn));
	}
}

// Registering a file that does not exist on disk tracks it with size zero;
// deleting it again succeeds because a missing file counts as deleted.
TEST_F(RenderTailDiskCacheTest, CreatedFileForMissingFileIsTrackedAsZeroSize)
{
	const QString sub = MakeSubDir(QStringLiteral("zero_size"));
	ASSERT_FALSE(sub.isEmpty());

	olive::DiskCacheFolder folder(sub);

	const QString ghost = QDir(sub).filePath(QStringLiteral("ghost"));
	ASSERT_FALSE(QFileInfo::exists(ghost));
	folder.CreatedFile(ghost);

	QSignalSpy spy(&folder, &olive::DiskCacheFolder::DeletedFrame);

	EXPECT_TRUE(folder.DeleteSpecificFile(ghost));

	ASSERT_EQ(spy.count(), 1);
	EXPECT_EQ(spy.first().at(0).toString(), sub);
	EXPECT_EQ(spy.first().at(1).toString(), ghost);
}

// DiskManager::Accessed/CreatedFile forward to the matching folder and
// DeleteSpecificFile broadcasts to every open folder, re-emitting the folder's
// DeletedFrame signal as its own.
TEST_F(RenderTailDiskCacheTest,
	   DiskManagerAccessedAndDeleteSpecificFileForwardToFolder)
{
	olive::DiskManager *dm = olive::DiskManager::instance();
	ASSERT_NE(dm, nullptr);

	const QString sub = MakeSubDir(QStringLiteral("forwarding"));
	ASSERT_FALSE(sub.isEmpty());

	const QString fn = QDir(sub).filePath(QStringLiteral("frame"));
	ASSERT_TRUE(WriteFile(fn, 32));

	dm->CreatedFile(sub, fn);
	dm->Accessed(sub, fn);
	ASSERT_TRUE(QFileInfo::exists(fn));

	QSignalSpy spy(dm, &olive::DiskManager::DeletedFrame);

	dm->DeleteSpecificFile(fn);

	EXPECT_FALSE(QFileInfo::exists(fn));
	ASSERT_EQ(spy.count(), 1);
	EXPECT_EQ(spy.first().at(0).toString(), sub);
	EXPECT_EQ(spy.first().at(1).toString(), fn);
}

// The static path helpers must return distinct, non-empty locations; the config
// file name is part of the on-disk format.
TEST_F(RenderTailDiskCacheTest, DefaultDiskCachePathsAreNonEmptyAndDistinct)
{
	const QString config_file =
		olive::DiskManager::GetDefaultDiskCacheConfigFile();
	const QString cache_path = olive::DiskManager::GetDefaultDiskCachePath();

	EXPECT_FALSE(config_file.isEmpty());
	EXPECT_FALSE(cache_path.isEmpty());
	EXPECT_NE(config_file, cache_path);
	EXPECT_TRUE(config_file.endsWith(QStringLiteral("defaultdiskcache")));
}

class RenderTailAudioCacheTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		if (!temp_dir_.isValid()) {
			GTEST_FAIL() << "Failed to create temporary directory";
		}

		if (!olive::Core::instance()) {
			new olive::Core(olive::Core::CoreParams()); // intentionally leaked
		}

		olive::DiskManager::CreateInstance();

		// Point the project cache at a folder alongside the (unsaved) project
		// file so every cache write stays inside the temporary directory.
		olive::ColorManager::SetUpDefaultConfig();
		project_ = std::make_unique<olive::Project>();
		project_->Initialize();
		project_->set_filename(
			QDir(temp_dir_.path()).filePath(QStringLiteral("test.ove")));
		project_->SetCacheLocationSetting(
			olive::Project::kCacheStoreAlongsideProject);
	}

	void TearDown() override
	{
		project_.reset();
		olive::DiskManager::DestroyInstance();
	}

	static olive::core::AudioParams MakeParams()
	{
		return olive::core::AudioParams(48000,
										olive::core::kChannelLayoutStereo,
										olive::core::SampleFormat::F32P);
	}

	static void FillBuffer(olive::core::SampleBuffer *buf, float ch0, float ch1)
	{
		for (size_t i = 0; i < buf->sample_count(); i++) {
			buf->data(0)[i] = ch0;
			buf->data(1)[i] = ch1;
		}
	}

	QTemporaryDir temp_dir_;
	std::unique_ptr<olive::Project> project_;
};

// SetParameters stores the audio params; setting the same value twice early-outs
// and leaves them untouched.
TEST_F(RenderTailAudioCacheTest, SetParametersRoundTrip)
{
	olive::AudioPlaybackCache cache(project_.get());
	EXPECT_EQ(cache.GetParameters().channel_count(), 0);

	const olive::core::AudioParams params = MakeParams();
	cache.SetParameters(params);
	EXPECT_EQ(cache.GetParameters(), params);

	cache.SetParameters(params);
	EXPECT_EQ(cache.GetParameters(), params);

	const olive::core::AudioParams other(44100,
										 olive::core::kChannelLayoutMono,
										 olive::core::SampleFormat::F32P);
	cache.SetParameters(other);
	EXPECT_EQ(cache.GetParameters().sample_rate(), 44100);
	EXPECT_EQ(cache.GetParameters().channel_count(), 1);
}

// WritePCM writes one segment file per channel, zero-padded to the full segment
// size, and validates exactly the written range.
TEST_F(RenderTailAudioCacheTest, WritePcmWritesSegmentFilesAndValidatesRange)
{
	olive::AudioPlaybackCache cache(project_.get());
	cache.SetParameters(MakeParams());

	const olive::TimeRange range(olive::rational(0), olive::rational(1, 10));

	olive::core::SampleBuffer buf(MakeParams(), olive::rational(1, 10));
	ASSERT_TRUE(buf.is_allocated());
	FillBuffer(&buf, 0.5f, 0.25f);

	cache.WritePCM(range, { range }, buf);

	EXPECT_TRUE(cache.HasValidatedRanges());
	EXPECT_FALSE(cache.HasInvalidatedRanges(range));

	// 4800 samples of 4-byte floats per channel
	const qint64 data_bytes = 19200;

	const QDir seg_dir = cache.GetThisCacheDirectory();
	const QString ch0 = seg_dir.filePath(QStringLiteral("0.0"));
	const QString ch1 = seg_dir.filePath(QStringLiteral("0.1"));
	ASSERT_TRUE(QFileInfo::exists(ch0));
	ASSERT_TRUE(QFileInfo::exists(ch1));

	// The buffer covered the whole range, so no padding is needed and the
	// segment contains exactly the written data
	EXPECT_EQ(QFileInfo(ch0).size(), data_bytes);
	EXPECT_EQ(QFileInfo(ch1).size(), data_bytes);

	QByteArray bytes;
	ASSERT_TRUE(ReadBytesAt(ch0, 0, 4, &bytes));
	EXPECT_FLOAT_EQ(BytesToFloat(bytes), 0.5f);

	ASSERT_TRUE(ReadBytesAt(ch1, 0, 4, &bytes));
	EXPECT_FLOAT_EQ(BytesToFloat(bytes), 0.25f);
}

// A write that does not start at zero must seek into the segment, leaving the
// preceding bytes as silence.
TEST_F(RenderTailAudioCacheTest, WritePcmAtNonZeroStartWritesAtByteOffset)
{
	olive::AudioPlaybackCache cache(project_.get());
	cache.SetParameters(MakeParams());

	const olive::TimeRange range(olive::rational(1, 10), olive::rational(1, 5));

	olive::core::SampleBuffer buf(MakeParams(), olive::rational(1, 10));
	ASSERT_TRUE(buf.is_allocated());
	FillBuffer(&buf, 0.75f, 0.75f);

	cache.WritePCM(range, { range }, buf);

	EXPECT_FALSE(cache.HasInvalidatedRanges(range));

	const qint64 data_bytes = 19200;
	const QString ch0 =
		cache.GetThisCacheDirectory().filePath(QStringLiteral("0.0"));
	ASSERT_TRUE(QFileInfo::exists(ch0));
	// The file extends exactly to the end of the written range
	EXPECT_EQ(QFileInfo(ch0).size(), 2 * data_bytes);

	// The first range was never written, so it reads back as silence
	QByteArray bytes;
	ASSERT_TRUE(ReadBytesAt(ch0, 0, 4, &bytes));
	EXPECT_EQ(bytes, QByteArray(4, '\0'));

	// The new data starts exactly at its byte offset
	ASSERT_TRUE(ReadBytesAt(ch0, data_bytes, 4, &bytes));
	EXPECT_FLOAT_EQ(BytesToFloat(bytes), 0.75f);
}

// Only the listed valid ranges are validated, even when the sample buffer
// covers the whole render range.
TEST_F(RenderTailAudioCacheTest, WritePcmWithPartialValidRangesValidatesOnlyThose)
{
	olive::AudioPlaybackCache cache(project_.get());
	cache.SetParameters(MakeParams());

	const olive::TimeRange range(olive::rational(0), olive::rational(1, 5));
	const olive::TimeRange first_half(olive::rational(0), olive::rational(1, 10));

	olive::core::SampleBuffer buf(MakeParams(), olive::rational(1, 5));
	ASSERT_TRUE(buf.is_allocated());
	FillBuffer(&buf, 0.5f, 0.5f);

	cache.WritePCM(range, { first_half }, buf);

	EXPECT_FALSE(cache.HasInvalidatedRanges(first_half));
	EXPECT_TRUE(cache.HasInvalidatedRanges(range));
}

// An empty valid-range list writes no segments and validates nothing.
TEST_F(RenderTailAudioCacheTest, WritePcmWithNoValidRangesWritesNothing)
{
	olive::AudioPlaybackCache cache(project_.get());
	cache.SetParameters(MakeParams());

	const olive::TimeRange range(olive::rational(0), olive::rational(1, 10));

	olive::core::SampleBuffer buf(MakeParams(), olive::rational(1, 10));
	ASSERT_TRUE(buf.is_allocated());

	cache.WritePCM(range, olive::TimeRangeList(), buf);

	EXPECT_FALSE(cache.HasValidatedRanges());
	EXPECT_FALSE(QFileInfo::exists(
		cache.GetThisCacheDirectory().filePath(QStringLiteral("0.0"))));
}

// A write larger than one segment must spill into the next segment file, with
// each touched segment zero-padded to its full extent.
TEST_F(RenderTailAudioCacheTest,
	   WritePcmSpanningSegmentBoundaryCreatesBothSegments)
{
	olive::AudioPlaybackCache cache(project_.get());
	cache.SetParameters(MakeParams());

	// 56 seconds at 48000 Hz is 10752000 bytes per channel, just over one
	// 10 MB segment.
	const olive::TimeRange range(olive::rational(0), olive::rational(56));

	olive::core::SampleBuffer buf(MakeParams(), olive::rational(56));
	ASSERT_TRUE(buf.is_allocated());
	ASSERT_EQ(buf.sample_count(), size_t(56 * 48000));
	FillBuffer(&buf, 1.0f, 1.0f);

	cache.WritePCM(range, { range }, buf);

	EXPECT_FALSE(cache.HasInvalidatedRanges(range));

	const QDir seg_dir = cache.GetThisCacheDirectory();
	const QString seg0 = seg_dir.filePath(QStringLiteral("0.0"));
	const QString seg1 = seg_dir.filePath(QStringLiteral("1.0"));
	ASSERT_TRUE(QFileInfo::exists(seg0));
	ASSERT_TRUE(QFileInfo::exists(seg1));

	EXPECT_EQ(QFileInfo(seg0).size(), kSegmentSize);
	// The second segment holds exactly the spillover bytes
	EXPECT_EQ(QFileInfo(seg1).size(),
			  56 * 48000 * 4 - kSegmentSize);

	// The spillover data starts at the beginning of the second segment file
	QByteArray bytes;
	ASSERT_TRUE(ReadBytesAt(seg1, 0, 4, &bytes));
	EXPECT_FLOAT_EQ(BytesToFloat(bytes), 1.0f);
}
