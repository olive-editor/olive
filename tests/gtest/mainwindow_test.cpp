#include <gtest/gtest.h>

#include <QProgressBar>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QSignalSpy>
#include <QTest>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "audio/audiomanager.h"
#include "config/config.h"
#include "node/color/colormanager/colormanager.h"
#include "core.h"
#include "node/output/viewer/viewer.h"
#include "node/project.h"
#include "node/project/folder/folder.h"
#include "node/project/sequence/sequence.h"
#include "panel/panelmanager.h"
#include "render/diskmanager.h"
#include "render/rendermanager.h"
#include "task/task.h"
#include "task/taskmanager.h"
#include "widget/menu/menushared.h"
#include "window/mainwindow/mainstatusbar.h"
#include "window/mainwindow/mainwindow.h"
#include "window/mainwindow/mainwindowlayoutinfo.h"

using namespace olive;

namespace
{

class DummyTask : public Task {
public:
	DummyTask()
	{
		SetTitle(QStringLiteral("Status Test Task"));
	}

protected:
	virtual bool Run() override
	{
		return true;
	}
};

} // namespace

TEST(MainWindowLayoutInfo, AccessorsStoreAndRetrieve)
{
	Project project;
	project.Initialize();
	auto *folder = new Folder();
	folder->setParent(&project);
	auto *sequence = new Sequence();
	sequence->setParent(&project);
	auto *viewer = new ViewerOutput();
	viewer->setParent(&project);

	MainWindowLayoutInfo info;
	EXPECT_TRUE(info.open_folders().empty());
	EXPECT_TRUE(info.open_sequences().empty());
	EXPECT_TRUE(info.open_viewers().empty());
	EXPECT_TRUE(info.panel_data().empty());
	EXPECT_TRUE(info.state().isEmpty());

	info.add_folder(folder);
	info.add_sequence(sequence);
	info.add_viewer(viewer);

	ASSERT_EQ(info.open_folders().size(), 1);
	EXPECT_EQ(info.open_folders().front(), folder);
	ASSERT_EQ(info.open_sequences().size(), 1);
	EXPECT_EQ(info.open_sequences().front(), sequence);
	ASSERT_EQ(info.open_viewers().size(), 1);
	EXPECT_EQ(info.open_viewers().front(), viewer);

	PanelWidget::Info data;
	data[QStringLiteral("key")] = QStringLiteral("value");
	info.set_panel_data(QStringLiteral("panel_a"), data);
	ASSERT_EQ(info.panel_data().size(), 1);
	EXPECT_EQ(info.panel_data()
				  .at(QStringLiteral("panel_a"))
				  .at(QStringLiteral("key")),
			  QStringLiteral("value"));

	// move_panel_data renames the entry
	info.move_panel_data(QStringLiteral("panel_a"),
						 QStringLiteral("panel_b"));
	EXPECT_EQ(info.panel_data().count(QStringLiteral("panel_a")), 0);
	ASSERT_EQ(info.panel_data().count(QStringLiteral("panel_b")), 1);
	EXPECT_EQ(info.panel_data()
				  .at(QStringLiteral("panel_b"))
				  .at(QStringLiteral("key")),
			  QStringLiteral("value"));

	info.set_state(QByteArray("layout-state"));
	EXPECT_EQ(info.state(), QByteArray("layout-state"));
}

TEST(MainWindowLayoutInfo, XmlRoundTripPreservesEverything)
{
	Project project;
	project.Initialize();
	auto *folder = new Folder();
	folder->setParent(&project);
	auto *sequence = new Sequence();
	sequence->setParent(&project);
	auto *viewer = new ViewerOutput();
	viewer->setParent(&project);

	MainWindowLayoutInfo info;
	info.add_folder(folder);
	info.add_sequence(sequence);
	info.add_viewer(viewer);
	PanelWidget::Info data;
	data[QStringLiteral("splitter")] = QStringLiteral("AAA=");
	info.set_panel_data(QStringLiteral("TimelinePanel"), data);
	info.set_state(QByteArray("binary\x01\x02state", 12));

	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("layout"));
	info.toXml(&writer);
	writer.writeEndElement();
	writer.writeEndDocument();

	QHash<quintptr, Node *> node_map;
	node_map.insert(reinterpret_cast<quintptr>(folder), folder);
	node_map.insert(reinterpret_cast<quintptr>(sequence), sequence);
	node_map.insert(reinterpret_cast<quintptr>(viewer), viewer);

	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("layout"));

	MainWindowLayoutInfo loaded = MainWindowLayoutInfo::fromXml(&reader, node_map);

	ASSERT_EQ(loaded.open_folders().size(), 1);
	EXPECT_EQ(loaded.open_folders().front(), folder);
	ASSERT_EQ(loaded.open_sequences().size(), 1);
	EXPECT_EQ(loaded.open_sequences().front(), sequence);

	// Open viewers must survive the round trip too
	ASSERT_EQ(loaded.open_viewers().size(), 1);
	EXPECT_EQ(loaded.open_viewers().front(), viewer);

	EXPECT_EQ(loaded.state(), info.state());

	ASSERT_EQ(loaded.panel_data().count(QStringLiteral("TimelinePanel")), 1);
	EXPECT_EQ(loaded.panel_data()
				  .at(QStringLiteral("TimelinePanel"))
				  .at(QStringLiteral("splitter")),
			  QStringLiteral("AAA="));

	// No unknown nodes leak into the viewers list: it must contain exactly the
	// viewer that was added, not a duplicate of the sequences list
	EXPECT_NE(loaded.open_viewers().front(),
			  static_cast<ViewerOutput *>(sequence));
}

TEST(MainWindowLayoutInfo, FromXmlSkipsUnknownElementsAndNodes)
{
	const QString xml = QStringLiteral(
		"<layout version=\"1\">"
		"<folders><folder>1</folder><unknown><nested/></unknown></folders>"
		"<timeline><sequence>2</sequence></timeline>"
		"<viewers><viewer>3</viewer></viewers>"
		"<mystery><deep><deeper/></deep></mystery>"
		"<state>QUJD</state>"
		"</layout>");

	// No node map entries: pointers resolve to nullptr
	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("layout"));

	MainWindowLayoutInfo info = MainWindowLayoutInfo::fromXml(&reader, {});

	// Unknown pointers resolve to null but are still listed
	ASSERT_EQ(info.open_folders().size(), 1);
	EXPECT_EQ(info.open_folders().front(), nullptr);
	ASSERT_EQ(info.open_sequences().size(), 1);
	EXPECT_EQ(info.open_sequences().front(), nullptr);
	ASSERT_EQ(info.open_viewers().size(), 1);
	EXPECT_EQ(info.open_viewers().front(), nullptr);
	EXPECT_EQ(info.state(), QByteArray("ABC"));
	EXPECT_TRUE(info.panel_data().empty());
}

TEST(MainWindowStatusBar, ConstructionDefaults)
{
	MainStatusBar bar;
	bar.show();

	auto *progress = bar.findChild<QProgressBar *>();
	ASSERT_NE(progress, nullptr);
	EXPECT_FALSE(progress->isVisible());
}

TEST(MainWindowStatusBar, ReflectsTaskManagerState)
{
	TaskManager manager;

	MainStatusBar bar;
	bar.show();
	auto *progress = bar.findChild<QProgressBar *>();
	ASSERT_NE(progress, nullptr);

	bar.ConnectTaskManager(&manager);

	auto *task = new DummyTask();
	manager.AddTask(task);

	// One running task shows its title and the progress bar
	EXPECT_EQ(bar.currentMessage(), QStringLiteral("Status Test Task"));
	EXPECT_TRUE(progress->isVisible());

	// Progress signals are forwarded to the bar
	emit task->ProgressChanged(0.5);
	EXPECT_EQ(progress->value(), 50);

	// When the task list empties, the bar hides and the message clears
	manager.CancelTaskAndWait(task);
	QTRY_COMPARE_WITH_TIMEOUT(manager.GetTaskCount(), 0, 2000);
	EXPECT_TRUE(bar.currentMessage().isEmpty());
	EXPECT_FALSE(progress->isVisible());
	EXPECT_EQ(progress->value(), 0);

	QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

TEST(MainWindowStatusBar, DoubleClickEmitsSignal)
{
	MainStatusBar bar;
	bar.show();

	QSignalSpy spy(&bar, &MainStatusBar::DoubleClicked);
	QTest::mouseDClick(&bar, Qt::LeftButton);
	EXPECT_GE(spy.count(), 1);
}

// Evaluation of full MainWindow construction under the offscreen platform.
// Core::StartGUI() is private, so this test replicates its relevant steps:
// it creates the singletons MainWindow depends on (MenuShared, PanelManager,
// AudioManager, DiskManager, plus TaskManager for the status bar and
// RenderManager for the viewer panels' display widgets) and then instantiates
// MainWindow directly.
TEST(MainWindow, ConstructsOffscreenWithPanelsAndMenus)
{
	// MainWindow instantiates viewer panels containing QOpenGLWidget. On
	// platforms without a usable OpenGL implementation (e.g. headless CI
	// runners on the offscreen QPA), constructing it crashes in GL code
	// ("QOpenGLFunctions created with non-current context"). Probe first and
	// skip where GL is unavailable.
	QOffscreenSurface probe_surface;
	probe_surface.create();
	QOpenGLContext probe_context;
	const bool gl_available =
		probe_context.create() && probe_context.makeCurrent(&probe_surface);
	probe_context.doneCurrent();
	if (!gl_available) {
		GTEST_SKIP() << "OpenGL is not usable on this platform";
	}

	// Must precede RenderManager creation: PreviewAutoCacher constructs a
	// Project whose ColorManager dereferences the default OCIO config
	ColorManager::SetUpDefaultConfig();

	const bool created_task_manager = (TaskManager::instance() == nullptr);
	if (created_task_manager) {
		TaskManager::CreateInstance();
	}
	const bool created_render_manager = (RenderManager::instance() == nullptr);
	QVariant saved_backend;
	if (created_render_manager) {
		// Another suite may have left an experimental backend in the config;
		// RenderManager needs a real one to create its cacher
		saved_backend = Config::Current()[QStringLiteral("GraphicsBackend")];
		Config::Current()[QStringLiteral("GraphicsBackend")] =
			QStringLiteral("opengl");
		RenderManager::CreateInstance();
	}
	const bool created_disk_manager = (DiskManager::instance() == nullptr);
	if (created_disk_manager) {
		DiskManager::CreateInstance();
	}
	const bool created_menu_shared = (MenuShared::instance() == nullptr);
	if (created_menu_shared) {
		MenuShared::CreateInstance();
	}
	const bool created_panel_manager = (PanelManager::instance() == nullptr);
	if (created_panel_manager) {
		PanelManager::CreateInstance();
	}
	const bool created_audio_manager = (AudioManager::instance() == nullptr);
	if (created_audio_manager) {
		AudioManager::CreateInstance();
	}
	if (!Core::instance()) {
		new Core(Core::CoreParams()); // intentionally leaked
	}
	KDDockWidgets::initFrontend(KDDockWidgets::FrontendType::QtWidgets);

	// Suppress the modal welcome dialog shown on first show
	const QVariant welcome_setting =
		Config::Current()[QStringLiteral("ShowWelcomeDialog")];
	Config::Current()[QStringLiteral("ShowWelcomeDialog")] = false;

	MainWindow *window = new MainWindow();
	window->showMaximized();

	// The standard panels were created and registered by name
	PanelManager *panels = PanelManager::instance();
	ASSERT_NE(panels, nullptr);
	EXPECT_GE(panels->panels().size(), 10);
	EXPECT_NE(panels->GetPanelWithName(QStringLiteral("NodePanel")), nullptr);
	EXPECT_NE(panels->GetPanelWithName(QStringLiteral("ProjectPanel")),
			  nullptr);
	// Timeline panels get their index appended to the unique name
	EXPECT_NE(panels->GetPanelWithName(QStringLiteral("TimelinePanel:0")),
			  nullptr);
	EXPECT_NE(panels->GetPanelWithName(QStringLiteral("SequenceViewerPanel")),
			  nullptr);
	EXPECT_NE(panels->GetPanelWithName(QStringLiteral("FootageViewerPanel")),
			  nullptr);

	// The menu bar is fully populated (this is what the action search dialog
	// indexes)
	QMenuBar *menu_bar = window->menuBar();
	ASSERT_NE(menu_bar, nullptr);
	int total_actions = 0;
	foreach (QAction *menu_action, menu_bar->actions()) {
		if (QMenu *menu = menu_action->menu()) {
			total_actions += menu->actions().size();
		}
	}
	EXPECT_GE(menu_bar->actions().size(), 5);
	EXPECT_GT(total_actions, 0);

	// Status bar and window title are set up
	EXPECT_NE(window->statusBar(), nullptr);
	EXPECT_FALSE(window->windowTitle().isEmpty());

	// The default layout restore is queued at construction; pumping the event
	// loop must not crash
	QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

	Config::Current()[QStringLiteral("ShowWelcomeDialog")] = welcome_setting;

	// Tear down in reverse order: window first, then its panels, then only
	// the singletons this test created
	delete window;
	if (created_panel_manager) {
		PanelManager::instance()->DeleteAllPanels();
		PanelManager::DestroyInstance();
	}
	if (created_audio_manager) {
		AudioManager::DestroyInstance();
	}
	if (created_menu_shared) {
		MenuShared::DestroyInstance();
	}
	if (created_render_manager) {
		RenderManager::DestroyInstance();
		Config::Current()[QStringLiteral("GraphicsBackend")] = saved_backend;
	}
	if (created_task_manager) {
		TaskManager::DestroyInstance();
	}
	if (created_disk_manager) {
		DiskManager::DestroyInstance();
	}
}
