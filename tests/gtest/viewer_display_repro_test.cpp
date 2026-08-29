/*
 * Regression test for the direct-connection black screen bug. Drives the real
 * ViewerWidget display path offscreen and checks the pixels the display
 * widget actually paints, for both the direct (footage -> output) and
 * indirect (footage -> effect -> output) wiring, plus runtime rewiring
 * between the two.
 */

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QImage>
#include <QPixmap>
#include <QThread>

#include "audio/audiomanager.h"
#include "codec/conformmanager.h"
#include "codec/proxymanager.h"
#include "config/config.h"
#include "core.h"
#include "node/color/colormanager/colormanager.h"
#include "node/effect/opacity/opacityeffect.h"
#include "node/factory.h"
#include "node/output/viewer/viewer.h"
#include "node/project.h"
#include "node/project/footage/footage.h"
#include "node/project/sequence/sequence.h"
#include "node/project/serializer/serializer.h"
#include "render/diskmanager.h"
#include "render/framemanager.h"
#include "render/previewautocacher.h"
#include "render/rendermanager.h"
#include "task/taskmanager.h"
#include "widget/viewer/footageviewer.h"
#include "widget/viewer/viewer.h"
#include "widget/viewer/viewerdisplay.h"

#ifdef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
#include "render/backend/dynamicrenderer.h"
#include "render/backend/renderbackend_c.h"
#endif

using namespace olive;

namespace
{

QString DemoVideoPathT()
{
	return QDir(QStringLiteral(OAK_TEST_SOURCE_DIR))
		.filePath(QStringLiteral("tests/demo.mp4"));
}

QString WorkerBinaryPathT()
{
	QDir dir(QCoreApplication::applicationDirPath());
	dir.cdUp();
	dir.cdUp();
	dir.cd(QStringLiteral("app"));
#if defined(_WIN32)
	return dir.filePath(QStringLiteral("oak-render-worker.exe"));
#else
	return dir.filePath(QStringLiteral("oak-render-worker"));
#endif
}

// Loads and initializes the requested render backend the same way the
// application does, verifying that the loaded backend is actually of the
// requested kind (a Vulkan request that fell back to OpenGL does not count).
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

double BrightnessOfWidget(QWidget *w)
{
	QPixmap pm = w->grab();
	QImage img = pm.toImage().convertToFormat(QImage::Format_RGB32);
	if (img.isNull()) {
		return -1.0;
	}
	double sum = 0;
	int n = 0;
	for (int y = img.height() / 4; y < img.height() * 3 / 4; y += 8) {
		for (int x = img.width() / 4; x < img.width() * 3 / 4; x += 8) {
			QRgb px = img.pixel(x, y);
			sum += qRed(px) + qGreen(px) + qBlue(px);
			n += 3;
		}
	}
	return n ? sum / n / 255.0 : -1.0;
}

class TestViewerWidget : public ViewerWidget {
public:
	using ViewerWidget::display_widget;
	using ViewerWidget::ViewerWidget;
};

class TestFootageViewerWidget : public FootageViewerWidget {
public:
	using FootageViewerWidget::display_widget;
	using FootageViewerWidget::FootageViewerWidget;
};

} // namespace

class ViewerDisplayReproTest : public ::testing::TestWithParam<QString> {
protected:
	static void SetUpTestSuite()
	{
		NodeFactory::Initialize();
		ColorManager::SetUpDefaultConfig();
		TaskManager::CreateInstance();
		ConformManager::CreateInstance();
		ProxyManager::CreateInstance();
		FrameManager::CreateInstance();
		ProjectSerializer::Initialize();
		DiskManager::CreateInstance();

		const QString worker = WorkerBinaryPathT();
		if (QFileInfo::exists(worker)) {
			qputenv("OAK_RENDER_WORKER", QFile::encodeName(worker));
		}

		if (!Core::instance()) {
			new Core(Core::CoreParams());
		}
		AudioManager::CreateInstance();
	}

	static void TearDownTestSuite()
	{
		AudioManager::DestroyInstance();
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
		if (backend_ != QStringLiteral("vulkan")) {
			GTEST_SKIP() << "offscreen QOpenGLWidget cannot paint; Vulkan only";
		}
		if (!IsRenderBackendAvailable(backend_)) {
			GTEST_SKIP() << "Render backend is not available: "
						 << backend_.toStdString();
		}
		Config::Current()[QStringLiteral("GraphicsBackend")] = backend_;

		demo_path_ = DemoVideoPathT();
		ASSERT_TRUE(QFileInfo::exists(demo_path_));

		project_ = std::make_unique<Project>();
		project_->Initialize();

		footage_ = new Footage(demo_path_);
		footage_->setParent(project_.get());
		ASSERT_TRUE(footage_->IsValid());

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

	// Pumps the event loop until the display widget has a texture or timeout.
	bool WaitForTexture(ViewerDisplayWidget *display, int timeout_ms = 30000)
	{
		QElapsedTimer timer;
		timer.start();
		while (!timer.hasExpired(timeout_ms)) {
			QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
			if (display->GetCurrentTexture()) {
				// Let a few more paints happen
				for (int i = 0; i < 5; i++) {
					QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
					QThread::msleep(20);
				}
				return true;
			}
			QThread::msleep(20);
		}
		return false;
	}

	QString backend_;
	QString demo_path_;
	std::unique_ptr<Project> project_;
	Footage *footage_ = nullptr;
};

// Footage previewed in the footage viewer (the "direct" case: the footage
// node itself is the viewer output, nothing in between).
TEST_P(ViewerDisplayReproTest, FootageViewerNotBlack)
{
	TestFootageViewerWidget *viewer = new TestFootageViewerWidget();
	viewer->resize(800, 600);
	viewer->show();

	viewer->ConnectViewerNode(footage_);

	ASSERT_TRUE(WaitForTexture(viewer->display_widget()))
		<< "Display widget never received a texture (backend="
		<< backend_.toStdString() << ")";

	double brightness = BrightnessOfWidget(viewer->display_widget());
	EXPECT_GT(brightness, 0.01)
		<< "Footage viewer paints BLACK (brightness=" << brightness
		<< ", backend=" << backend_.toStdString() << ")";

	delete viewer;
}

// Sequence with footage -> opacity -> sequence output (the "indirect" case).
TEST_P(ViewerDisplayReproTest, SequenceViewerIndirectNotBlack)
{
	Sequence *sequence = new Sequence();
	sequence->setParent(project_.get());

	OpacityEffect *opacity = new OpacityEffect();
	opacity->setParent(project_.get());

	Node::ConnectEdge(footage_, NodeInput(opacity, OpacityEffect::kTextureInput));
	Node::ConnectEdge(opacity, NodeInput(sequence, ViewerOutput::kTextureInput));

	TestViewerWidget *viewer = new TestViewerWidget();
	viewer->resize(800, 600);
	viewer->show();

	viewer->ConnectViewerNode(sequence);

	ASSERT_TRUE(WaitForTexture(viewer->display_widget()))
		<< "Display widget never received a texture (backend="
		<< backend_.toStdString() << ")";

	double brightness = BrightnessOfWidget(viewer->display_widget());
	EXPECT_GT(brightness, 0.01)
		<< "Sequence viewer (indirect) paints BLACK (brightness=" << brightness
		<< ", backend=" << backend_.toStdString() << ")";

	delete viewer;
}

// Sequence with footage wired directly to the output (node-editor "direct"
// case).
TEST_P(ViewerDisplayReproTest, SequenceViewerDirectNotBlack)
{
	Sequence *sequence = new Sequence();
	sequence->setParent(project_.get());

	Node::ConnectEdge(footage_,
					  NodeInput(sequence, ViewerOutput::kTextureInput));

	TestViewerWidget *viewer = new TestViewerWidget();
	viewer->resize(800, 600);
	viewer->show();

	viewer->ConnectViewerNode(sequence);

	ASSERT_TRUE(WaitForTexture(viewer->display_widget()))
		<< "Display widget never received a texture (backend="
		<< backend_.toStdString() << ")";

	double brightness = BrightnessOfWidget(viewer->display_widget());
	EXPECT_GT(brightness, 0.01)
		<< "Sequence viewer (direct) paints BLACK (brightness=" << brightness
		<< ", backend=" << backend_.toStdString() << ")";

	delete viewer;
}

INSTANTIATE_TEST_SUITE_P(Backends, ViewerDisplayReproTest,
						 ::testing::Values(QStringLiteral("opengl"),
										   QStringLiteral("vulkan")));

// Simulates the user's actual workflow: a sequence is playing through its
// normal chain, then at RUNTIME the input is rewired to connect the footage
// directly to the output. Vulkan only (offscreen QOpenGLWidget cannot paint).
class ViewerRuntimeRewireTest : public ViewerDisplayReproTest {
protected:
	double PumpAndMeasure(TestViewerWidget *viewer, int timeout_ms = 30000)
	{
		if (!WaitForTexture(viewer->display_widget(), timeout_ms)) {
			return -1.0;
		}
		return BrightnessOfWidget(viewer->display_widget());
	}
};

TEST_P(ViewerRuntimeRewireTest, RewireToDirectConnectionNotBlack)
{
	Sequence *sequence = new Sequence();
	sequence->setParent(project_.get());

	// Normal chain with a node in between: footage -> opacity -> sequence
	OpacityEffect *opacity = new OpacityEffect();
	opacity->setParent(project_.get());
	Node::ConnectEdge(footage_,
					  NodeInput(opacity, OpacityEffect::kTextureInput));
	Node::ConnectEdge(opacity, NodeInput(sequence, ViewerOutput::kTextureInput));

	TestViewerWidget *viewer = new TestViewerWidget();
	viewer->resize(800, 600);
	viewer->show();
	viewer->ConnectViewerNode(sequence);

	double brightness = PumpAndMeasure(viewer);
	ASSERT_GT(brightness, 0.01)
		<< "Precondition failed: indirect chain is already black";

	// Now rewire at runtime: footage directly to the sequence output.
	Node::DisconnectEdge(opacity,
						 NodeInput(sequence, ViewerOutput::kTextureInput));
	Node::ConnectEdge(footage_,
					  NodeInput(sequence, ViewerOutput::kTextureInput));

	brightness = PumpAndMeasure(viewer);
	EXPECT_GT(brightness, 0.01)
		<< "Viewer paints BLACK after rewiring to a direct connection "
		<< "(brightness=" << brightness << ")";

	delete viewer;
}

TEST_P(ViewerRuntimeRewireTest, RewireToIndirectConnectionNotBlack)
{
	Sequence *sequence = new Sequence();
	sequence->setParent(project_.get());

	// Start direct: footage -> sequence
	Node::ConnectEdge(footage_,
					  NodeInput(sequence, ViewerOutput::kTextureInput));

	TestViewerWidget *viewer = new TestViewerWidget();
	viewer->resize(800, 600);
	viewer->show();
	viewer->ConnectViewerNode(sequence);

	double brightness = PumpAndMeasure(viewer);
	ASSERT_GT(brightness, 0.01)
		<< "Precondition failed: direct chain is already black";

	// Insert a node at runtime: footage -> opacity -> sequence
	OpacityEffect *opacity = new OpacityEffect();
	opacity->setParent(project_.get());
	Node::DisconnectEdge(footage_,
						 NodeInput(sequence, ViewerOutput::kTextureInput));
	Node::ConnectEdge(footage_,
					  NodeInput(opacity, OpacityEffect::kTextureInput));
	Node::ConnectEdge(opacity, NodeInput(sequence, ViewerOutput::kTextureInput));

	brightness = PumpAndMeasure(viewer);
	EXPECT_GT(brightness, 0.01)
		<< "Viewer paints BLACK after rewiring to an indirect connection "
		<< "(brightness=" << brightness << ")";

	delete viewer;
}

INSTANTIATE_TEST_SUITE_P(Backends, ViewerRuntimeRewireTest,
						 ::testing::Values(QStringLiteral("opengl"),
										   QStringLiteral("vulkan")));
