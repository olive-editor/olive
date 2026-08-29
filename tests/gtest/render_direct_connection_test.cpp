/*
 * Oak Video Editor - Direct-Connection Preview Black Screen Regression Test
 * Copyright (C) 2026 Oak Team
 *
 * Reproduces the pre-existing bug where the preview is black when a footage
 * node is connected directly to a viewer output, while inserting any node in
 * between renders correctly. Drives the exact application render path:
 * PreviewAutoCacher -> RenderManager -> RenderWorkerPool -> oak-render-worker.
 */

#include <gtest/gtest.h>

#include <atomic>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QThread>

#include "codec/conformmanager.h"
#include "codec/frame.h"
#include "codec/proxymanager.h"
#include "config/config.h"
#include "node/color/colormanager/colormanager.h"
#include "node/distort/transform/transformdistortnode.h"
#include "node/effect/opacity/opacityeffect.h"
#include "node/factory.h"
#include "node/output/viewer/viewer.h"
#include "node/project.h"
#include "node/project/footage/footage.h"
#include "node/project/serializer/serializer.h"
#include "render/diskmanager.h"
#include "render/framemanager.h"
#include "render/previewautocacher.h"
#include "render/rendermanager.h"
#include "render/renderticket.h"
#include "task/taskmanager.h"

#ifdef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
#include "render/backend/dynamicrenderer.h"
#include "render/backend/renderbackend_c.h"
#endif

using namespace olive;

namespace
{

QString DemoVideoPath()
{
	return QDir(QStringLiteral(OAK_TEST_SOURCE_DIR))
		.filePath(QStringLiteral("tests/demo.mp4"));
}

QString WorkerBinaryPath()
{
	// The test binary lives in cmake-build-debug/tests/gtest; the worker is in
	// cmake-build-debug/app.
	QDir dir(QCoreApplication::applicationDirPath());
	dir.cdUp(); // tests/gtest -> tests
	dir.cdUp(); // tests -> build dir
	dir.cd(QStringLiteral("app"));
#if defined(_WIN32)
	return dir.filePath(QStringLiteral("oak-render-worker.exe"));
#else
	return dir.filePath(QStringLiteral("oak-render-worker"));
#endif
}

bool IsRenderBackendAvailable(const QString &backend)
{
#ifdef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
	olive::DynamicRenderer renderer(backend);
	if (!renderer.Load()) {
		return false;
	}

	OakRenderBackendInfo info = {};
	if (!renderer.GetBackendInfo(&info)) {
		return false;
	}

	if (backend == QStringLiteral("vulkan") &&
		info.kind != OAK_RENDER_BACKEND_VULKAN) {
		return false;
	}

	if (backend == QStringLiteral("opengl") &&
		info.kind != OAK_RENDER_BACKEND_OPENGL) {
		return false;
	}

	return renderer.Init();
#else
	Q_UNUSED(backend)
	return false;
#endif
}

// Returns the number of non-zero bytes sampled from the frame buffer, or -1
// when the frame is invalid.
int CountNonZeroBytes(const FramePtr &frame)
{
	if (!frame || !frame->is_allocated()) {
		return -1;
	}
	const auto *data = reinterpret_cast<const uint8_t *>(frame->const_data());
	const int size = frame->allocated_size();
	int nonzero = 0;
	// Sample across the buffer to keep the check fast.
	const int step = qMax(1, size / 4096);
	for (int i = 0; i < size; i += step) {
		if (data[i] != 0) {
			nonzero++;
		}
	}
	return nonzero;
}

} // namespace

class RenderDirectConnectionTest
	: public ::testing::Test,
	  public ::testing::WithParamInterface<QString> {
protected:
	static void SetUpTestSuite()
	{
		// Mirror Core::Start()'s singleton initialization order (RenderManager
		// is created per-test instead, so that each backend gets a fresh one).
		NodeFactory::Initialize();
		ColorManager::SetUpDefaultConfig();
		TaskManager::CreateInstance();
		ConformManager::CreateInstance();
		ProxyManager::CreateInstance();
		FrameManager::CreateInstance();
		ProjectSerializer::Initialize();
		DiskManager::CreateInstance();

		// Point the worker pool at the built worker binary.
		const QString worker = WorkerBinaryPath();
		if (QFileInfo::exists(worker)) {
			qputenv("OAK_RENDER_WORKER", QFile::encodeName(worker));
		}
	}

	static void TearDownTestSuite()
	{
		DiskManager::DestroyInstance();
		ProjectSerializer::Destroy();
		FrameManager::DestroyInstance();
		ProxyManager::DestroyInstance();
		ConformManager::DestroyInstance();
		TaskManager::DestroyInstance();
		NodeFactory::Destroy();
	}

	void SetUp() override
	{
		backend_ = GetParam();
		if (!IsRenderBackendAvailable(backend_)) {
			GTEST_SKIP() << "Render backend is not available: "
						 << backend_.toStdString();
		}

		const QString worker = WorkerBinaryPath();
		if (!QFileInfo::exists(worker)) {
			GTEST_SKIP() << "worker binary not found at "
						 << worker.toStdString();
		}

		demo_path_ = DemoVideoPath();
		ASSERT_TRUE(QFileInfo::exists(demo_path_));

		Config::Current()[QStringLiteral("GraphicsBackend")] = backend_;

		project_ = std::make_unique<Project>();
		project_->Initialize();

		footage_ = new Footage(demo_path_);
		footage_->setParent(project_.get());
		ASSERT_TRUE(footage_->IsValid())
			<< "Footage failed to probe " << demo_path_.toStdString();

		RenderManager::CreateInstance();
		RenderManager::instance()->GetCacher()->SetProject(project_.get());
	}

	void TearDown() override
	{
		// May be null when SetUp() skipped before creating the instance.
		if (RenderManager::instance()) {
			RenderManager::instance()->GetCacher()->SetProject(nullptr);
			RenderManager::DestroyInstance();
		}
		project_.reset();
	}

	// Renders one frame through the application's preview path and returns the
	// resulting CPU frame (nullptr on failure/timeout).
	FramePtr RenderOneFrame(ViewerOutput *viewer)
	{
		RenderTicketPtr ticket =
			RenderManager::instance()->GetCacher()->GetSingleFrame(
				viewer, rational(0), false);
		if (!ticket) {
			return nullptr;
		}

		std::atomic<bool> finished{ false };
		QObject::connect(ticket.get(), &RenderTicket::Finished,
						 [&finished]() { finished = true; });

		QElapsedTimer timer;
		timer.start();
		while (!finished.load() && !timer.hasExpired(60000)) {
			QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
			QThread::msleep(5);
		}

		if (!finished.load() || !ticket->HasResult()) {
			return nullptr;
		}

		return ticket->Get().value<FramePtr>();
	}

	QString backend_;
	QString demo_path_;
	std::unique_ptr<Project> project_;
	Footage *footage_ = nullptr;
};

// Footage previewed directly (footage node connected straight to the viewer
// output, no intermediate node) must not produce a black frame.
TEST_P(RenderDirectConnectionTest, DirectConnectionIsNotBlack)
{
	ViewerOutput *viewer = new ViewerOutput();
	viewer->setParent(project_.get());

	// Direct connection: footage -> viewer
	Node::ConnectEdge(footage_, NodeInput(viewer, ViewerOutput::kTextureInput));

	FramePtr frame = RenderOneFrame(viewer);
	ASSERT_TRUE(frame != nullptr)
		<< "Direct connection render produced no frame (timeout or empty "
		   "ticket)";
	ASSERT_TRUE(frame->is_allocated());

	int nonzero = CountNonZeroBytes(frame);
	EXPECT_GT(nonzero, 0)
		<< "Direct connection render is BLACK (all sampled bytes are zero)";

	Node::DisconnectEdge(footage_,
						 NodeInput(viewer, ViewerOutput::kTextureInput));
}

// Same as above but with an effect node between footage and viewer. This is
// the control case that reportedly works.
TEST_P(RenderDirectConnectionTest, IndirectConnectionIsNotBlack)
{
	ViewerOutput *viewer = new ViewerOutput();
	viewer->setParent(project_.get());

	OpacityEffect *opacity = new OpacityEffect();
	opacity->setParent(project_.get());

	// Indirect connection: footage -> opacity -> viewer
	Node::ConnectEdge(footage_,
					  NodeInput(opacity, OpacityEffect::kTextureInput));
	Node::ConnectEdge(opacity, NodeInput(viewer, ViewerOutput::kTextureInput));

	FramePtr frame = RenderOneFrame(viewer);
	ASSERT_TRUE(frame != nullptr) << "Indirect connection render produced no "
									 "frame (timeout or empty ticket)";
	ASSERT_TRUE(frame->is_allocated());

	int nonzero = CountNonZeroBytes(frame);
	EXPECT_GT(nonzero, 0)
		<< "Indirect connection render is BLACK (all sampled bytes are zero)";
}

INSTANTIATE_TEST_SUITE_P(Backends, RenderDirectConnectionTest,
						 ::testing::Values(QStringLiteral("opengl"),
										   QStringLiteral("vulkan")));

// Resolution-mismatch scenarios: footage is 1920x1080 while the viewer is
// 1280x720, forcing a rescale when the frame is downloaded in the worker.
class RenderResolutionMismatchTest : public RenderDirectConnectionTest {
protected:
	ViewerOutput *CreateSmallViewer()
	{
		ViewerOutput *viewer = new ViewerOutput();
		viewer->setParent(project_.get());
		viewer->SetVideoParams(VideoParams(
			1280, 720, rational(24),
			static_cast<PixelFormat::Format>(
				Config::Current()[QStringLiteral("OfflinePixelFormat")]
					.toInt()),
			VideoParams::kInternalChannelCount, rational(1),
			VideoParams::kInterlaceNone, 1));
		return viewer;
	}

	void ExpectFrameNotBlack(FramePtr frame, const char *what)
	{
		ASSERT_TRUE(frame != nullptr)
			<< what << " render produced no frame (timeout or empty ticket)";
		ASSERT_TRUE(frame->is_allocated());
		EXPECT_GT(CountNonZeroBytes(frame), 0)
			<< what << " render is BLACK (all sampled bytes are zero)";
	}
};

// Direct connection with mismatched resolutions: the worker must rescale the
// footage-sized texture into the viewer-sized output frame.
TEST_P(RenderResolutionMismatchTest, DirectConnectionNotBlack)
{
	ViewerOutput *viewer = CreateSmallViewer();
	Node::ConnectEdge(footage_, NodeInput(viewer, ViewerOutput::kTextureInput));

	ExpectFrameNotBlack(RenderOneFrame(viewer),
						"Direct connection (resolution mismatch)");

	Node::DisconnectEdge(footage_,
						 NodeInput(viewer, ViewerOutput::kTextureInput));
}

// Opacity passes the footage-sized texture through, so the worker still has
// to rescale at download time. Control case for the direct test above.
TEST_P(RenderResolutionMismatchTest, IndirectOpacityNotBlack)
{
	ViewerOutput *viewer = CreateSmallViewer();

	OpacityEffect *opacity = new OpacityEffect();
	opacity->setParent(project_.get());

	Node::ConnectEdge(footage_,
					  NodeInput(opacity, OpacityEffect::kTextureInput));
	Node::ConnectEdge(opacity, NodeInput(viewer, ViewerOutput::kTextureInput));

	ExpectFrameNotBlack(RenderOneFrame(viewer),
						"Indirect opacity (resolution mismatch)");
}

// Transform with auto-scale renders at the sequence resolution, so no size
// rescale is needed at download time. This mirrors the timeline chain.
TEST_P(RenderResolutionMismatchTest, IndirectTransformNotBlack)
{
	ViewerOutput *viewer = CreateSmallViewer();

	TransformDistortNode *transform = new TransformDistortNode();
	transform->setParent(project_.get());
	// 1 = Fit
	transform->SetStandardValue(TransformDistortNode::kAutoscaleInput, 1);

	Node::ConnectEdge(footage_,
					  NodeInput(transform, TransformDistortNode::kTextureInput));
	Node::ConnectEdge(transform,
					  NodeInput(viewer, ViewerOutput::kTextureInput));

	ExpectFrameNotBlack(RenderOneFrame(viewer),
						"Indirect transform (resolution mismatch)");
}

INSTANTIATE_TEST_SUITE_P(Backends, RenderResolutionMismatchTest,
						 ::testing::Values(QStringLiteral("opengl"),
										   QStringLiteral("vulkan")));
