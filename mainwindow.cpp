#include "mainwindow.h"

#include "io/config.h"
#include "io/path.h"

#include "project/footage.h"
#include "project/sequence.h"
#include "project/clip.h"
#include "project/undo.h"
#include "project/media.h"

#include "ui/sourcetable.h"
#include "ui/viewerwidget.h"
#include "ui/sourceiconview.h"
#include "ui/timelineheader.h"

#include "panels/panels.h"
#include "panels/project.h"
#include "panels/effectcontrols.h"
#include "panels/viewer.h"
#include "panels/timeline.h"
#include "panels/grapheditor.h"

#include "dialogs/aboutdialog.h"
#include "dialogs/newsequencedialog.h"
#include "dialogs/exportdialog.h"
#include "dialogs/preferencesdialog.h"
#include "dialogs/demonotice.h"
#include "dialogs/speeddialog.h"
#include "dialogs/actionsearch.h"

#include "playback/audio.h"
#include "playback/playback.h"

#include "debug.h"

#include <QStyleFactory>
#include <QMessageBox>
#include <QFileDialog>
#include <QTimer>
#include <QCloseEvent>
#include <QMovie>
#include <QInputDialog>
#include <QRegExp>
#include <QSortFilterProxyModel>
#include <QStatusBar>
#include <QMenu>
#include <QMenuBar>
#include <QLayout>
#include <QApplication>
#include <QPushButton>

MainWindow* mainWindow;

#define OLIVE_FILE_FILTER "Olive Project (*.ove)"

QTimer autorecovery_timer;
QString config_fn;
bool demoNoticeShown = false;

void MainWindow::setup_layout(bool reset) {
	panel_project->show();
	panel_effect_controls->show();
	panel_footage_viewer->show();
	panel_sequence_viewer->show();
	panel_timeline->show();
	panel_graph_editor->hide();

	addDockWidget(Qt::TopDockWidgetArea, panel_project);
	addDockWidget(Qt::TopDockWidgetArea, panel_footage_viewer);
	tabifyDockWidget(panel_footage_viewer, panel_effect_controls);
	panel_footage_viewer->raise();
	addDockWidget(Qt::TopDockWidgetArea, panel_sequence_viewer);
	addDockWidget(Qt::BottomDockWidgetArea, panel_timeline);
	panel_graph_editor->setFloating(true);

// workaround for strange Qt dock bug (see https://bugreports.qt.io/browse/QTBUG-65592)
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	resizeDocks({panel_project}, {40}, Qt::Horizontal);
#endif

	// load panels from file
	if (!reset) {
		QFile panel_config(get_config_path() + "/layout");
		if (panel_config.exists() && panel_config.open(QFile::ReadOnly)) {
			restoreState(panel_config.readAll(), 0);
			panel_config.close();
		}
	}

	layout()->update();
}

MainWindow::MainWindow(QWidget *parent, const QString &an) :
	QMainWindow(parent),
	appName(an)
{
	enable_launch_with_project = false;

	setup_debug();

	mainWindow = this;

	// set up style?

	qApp->setStyle(QStyleFactory::create("Fusion"));
	setStyleSheet("QPushButton::checked { background: rgb(25, 25, 25); }");

	QPalette darkPalette;
	darkPalette.setColor(QPalette::Window, QColor(53,53,53));
	darkPalette.setColor(QPalette::WindowText, Qt::white);
	darkPalette.setColor(QPalette::Base, QColor(25,25,25));
	darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
	darkPalette.setColor(QPalette::ToolTipBase, QColor(25,25,25));
	darkPalette.setColor(QPalette::ToolTipText, Qt::white);
	darkPalette.setColor(QPalette::Text, Qt::white);
	darkPalette.setColor(QPalette::Button, QColor(53,53,53));
	darkPalette.setColor(QPalette::ButtonText, Qt::white);	darkPalette.setColor(QPalette::BrightText, Qt::red);
	darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));
	darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
	darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
	darkPalette.setColor(QPalette::HighlightedText, Qt::black);

	qApp->setPalette(darkPalette);

	// end style
	QWidget* centralWidget = new QWidget(this);
	centralWidget->setMaximumSize(QSize(0, 0));
	setCentralWidget(centralWidget);

	setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

	setDockNestingEnabled(true);

	layout()->invalidate();

	QString data_dir = get_data_path();
	if (!data_dir.isEmpty()) {
		QDir dir(data_dir);
		dir.mkpath(".");
		if (dir.exists()) {
			qint64 a_month_ago = QDateTime::currentMSecsSinceEpoch() - 2592000000;
			qint64 a_week_ago = QDateTime::currentMSecsSinceEpoch() - 604800000;

			// TODO put delete functions in another thread?

			// delete auto-recoveries older than 7 days
			QStringList old_autorecoveries = dir.entryList(QStringList("autorecovery.ove.*"), QDir::Files);
			int deleted_ars = 0;
			for (int i=0;i<old_autorecoveries.size();i++) {
				QString file_name = data_dir + "/" + old_autorecoveries.at(i);
				qint64 file_time = QFileInfo(file_name).lastModified().toMSecsSinceEpoch();
				if (file_time < a_week_ago) {
					if (QFile(file_name).remove()) deleted_ars++;
				}
			}
			if (deleted_ars > 0) dout << "[INFO] Deleted" << deleted_ars << "autorecovery" << ((deleted_ars == 1) ? "file that was" : "files that were") << "older than 7 days";

			// delete previews older than 30 days
			QDir preview_dir = QDir(data_dir + "/previews");
			if (preview_dir.exists()) {
				deleted_ars = 0;
				QStringList old_prevs = preview_dir.entryList(QDir::Files);
				for (int i=0;i<old_prevs.size();i++) {
					QString file_name = preview_dir.absolutePath() + "/" + old_prevs.at(i);
					qint64 file_time = QFileInfo(file_name).lastRead().toMSecsSinceEpoch();
					if (file_time < a_month_ago) {
						if (QFile(file_name).remove()) deleted_ars++;
					}
				}
				if (deleted_ars > 0) dout << "[INFO] Deleted" << deleted_ars << "preview" << ((deleted_ars == 1) ? "file that was" : "files that were") << "last read over 30 days ago";
			}

			// search for open recents list
			recent_proj_file = data_dir + "/recents";
			QFile f(recent_proj_file);
			if (f.exists() && f.open(QFile::ReadOnly | QFile::Text)) {
				QTextStream text_stream(&f);
				while (true) {
					QString line = text_stream.readLine();
					if (line.isNull()) {
						break;
					} else {
						recent_projects.append(line);
					}
				}
				f.close();
			}
		}
	}
	QString config_path = get_config_path();
	if (!config_path.isEmpty()) {
		QDir config_dir(config_path);
		config_dir.mkpath(".");
		config_fn = config_path + "/config.xml";
		if (QFileInfo::exists(config_fn)) {
			config.load(config_fn);
		}
	}

	alloc_panels(this);

	QStatusBar* statusBar = new QStatusBar(this);
	statusBar->showMessage("Welcome to " + appName);
	setStatusBar(statusBar);

	setup_menus();

	if (!data_dir.isEmpty()) {
		// detect auto-recovery file
		autorecovery_filename = data_dir + "/autorecovery.ove";
		if (QFile::exists(autorecovery_filename)) {
			if (QMessageBox::question(NULL, "Auto-recovery", "Olive didn't close properly and an autorecovery file was detected. Would you like to open it?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
				enable_launch_with_project = false;
				open_project_worker(autorecovery_filename, true);
			}
		}
		autorecovery_timer.setInterval(60000);
		QObject::connect(&autorecovery_timer, SIGNAL(timeout()), this, SLOT(autorecover_interval()));
		autorecovery_timer.start();
	}

	setup_layout(false);

	init_audio();
}

MainWindow::~MainWindow() {
	free_panels();
	close_debug();
}

void MainWindow::launch_with_project(const QString& s) {
	project_url = s;
	enable_launch_with_project = true;
	demoNoticeShown = true;
}

void MainWindow::make_new_menu(QMenu *parent) {
	parent->addAction("&Project", this, SLOT(new_project()), QKeySequence("Ctrl+N"));
	parent->addSeparator();
	parent->addAction("&Sequence", this, SLOT(new_sequence()), QKeySequence("Ctrl+Shift+N"));
	parent->addAction("&Folder", this, SLOT(new_folder()));
}

void MainWindow::make_inout_menu(QMenu *parent) {
	parent->addAction("Set In Point", this, SLOT(set_in_point()), QKeySequence("I"));
	parent->addAction("Set Out Point", this, SLOT(set_out_point()), QKeySequence("O"));
	parent->addAction("Enable/Disable In/Out Point", this, SLOT(enable_inout()));
	parent->addSeparator();
	parent->addAction("Reset In Point", this, SLOT(clear_in()));
	parent->addAction("Reset Out Point", this, SLOT(clear_out()));
	parent->addAction("Clear In/Out Point", this, SLOT(clear_inout()), QKeySequence("G"));
}

void kbd_shortcut_processor(QByteArray& file, QMenu* menu, bool save, bool first) {
	QList<QAction*> actions = menu->actions();
	for (int i=0;i<actions.size();i++) {
		QAction* a = actions.at(i);
		if (a->menu() != NULL) {
			kbd_shortcut_processor(file, a->menu(), save, first);
		} else if (!a->isSeparator()) {
			if (save) {
				// saving custom shortcuts
				if (!a->property("default").isNull()) {
					QKeySequence defks(a->property("default").toString());
					if (a->shortcut() != defks) {
						// custom shortcut
						if (!file.isEmpty()) file.append('\n');
						file.append(a->text().replace("&", ""));
						file.append('\t');
						file.append(a->shortcut().toString());
					}
				}
			} else {
				// loading custom shortcuts
				if (first) {
					// store default shortcut
					a->setProperty("default", a->shortcut().toString());
				} else {
					// restore default shortcut
					a->setShortcut(a->property("default").toString());
				}
				QString comp_str = a->text().replace("&", "");
				int shortcut_index = file.indexOf(comp_str);
				if (shortcut_index == 0 || (shortcut_index > 0 && file.at(shortcut_index-1) == '\n')) {
					shortcut_index += comp_str.size() + 1;
					QString shortcut;
					while (shortcut_index < file.size() && file.at(shortcut_index) != '\n') {
						shortcut.append(file.at(shortcut_index));
						shortcut_index++;
					}
					QKeySequence ks(shortcut);
					if (!ks.isEmpty()) {
						a->setShortcut(ks);
					}
				}
			}
		}
	}
}

void MainWindow::load_shortcuts(const QString& fn, bool first) {
	QByteArray shortcut_bytes;
	QFile shortcut_path(fn);
	if (shortcut_path.exists() && shortcut_path.open(QFile::ReadOnly)) {
		shortcut_bytes = shortcut_path.readAll();
		shortcut_path.close();
	}
	QList<QAction*> menus = menuBar()->actions();
	for (int i=0;i<menus.size();i++) {
		QMenu* menu = menus.at(i)->menu();
		kbd_shortcut_processor(shortcut_bytes, menu, false, first);
	}
}

void MainWindow::save_shortcuts(const QString& fn) {
	// save main menu actions
	QList<QAction*> menus = menuBar()->actions();
	QByteArray shortcut_file;
	for (int i=0;i<menus.size();i++) {
		QMenu* menu = menus.at(i)->menu();
		kbd_shortcut_processor(shortcut_file, menu, true, false);
	}
	QFile shortcut_file_io(fn);
	if (shortcut_file_io.open(QFile::WriteOnly)) {
		shortcut_file_io.write(shortcut_file);
		shortcut_file_io.close();
	} else {
		dout << "[ERROR] Failed to save shortcut file";
	}
}

void MainWindow::show_about() {
	AboutDialog a(this);
	a.exec();
}

void MainWindow::delete_slot() {
	if (panel_timeline->headers->hasFocus()) {
		panel_timeline->headers->delete_markers();
	} else if (panel_timeline->focused()) {
		panel_timeline->delete_selection(sequence->selections, false);
	} else if (panel_effect_controls->is_focused()) {
		panel_effect_controls->delete_effects();
	} else if (panel_project->is_focused()) {
		panel_project->delete_selected_media();
	} else if (panel_effect_controls->keyframe_focus()) {
		panel_effect_controls->delete_selected_keyframes();
	} else if (panel_graph_editor->view_is_focused()) {
		panel_graph_editor->delete_selected_keys();
	}
}

void MainWindow::select_all() {
	if (panel_timeline->focused()) {
		panel_timeline->select_all();
	} else if (panel_graph_editor->view_is_focused()) {
		panel_graph_editor->select_all();
	}
}

void MainWindow::new_sequence() {
	NewSequenceDialog nsd(this);
	nsd.set_sequence_name(panel_project->get_next_sequence_name());
	nsd.exec();
}

void MainWindow::zoom_in() {
	QDockWidget* focused_panel = get_focused_panel();
	if (focused_panel == panel_timeline) {
		panel_timeline->set_zoom(true);
	} else if (focused_panel == panel_effect_controls) {
		panel_effect_controls->set_zoom(true);
	} else if (focused_panel == panel_footage_viewer) {
		panel_footage_viewer->set_zoom(true);
	} else if (focused_panel == panel_sequence_viewer) {
		panel_sequence_viewer->set_zoom(true);
	}
}

void MainWindow::zoom_out() {
	QDockWidget* focused_panel = get_focused_panel();
	if (focused_panel == panel_timeline) {
		panel_timeline->set_zoom(false);
	} else if (focused_panel == panel_effect_controls) {
		panel_effect_controls->set_zoom(false);
	} else if (focused_panel == panel_footage_viewer) {
		panel_footage_viewer->set_zoom(false);
	} else if (focused_panel == panel_sequence_viewer) {
		panel_sequence_viewer->set_zoom(false);
	}
}

void MainWindow::export_dialog() {
	if (sequence == NULL) {
		QMessageBox::information(this, "No active sequence", "Please open the sequence you wish to export.", QMessageBox::Ok);
	} else {
		ExportDialog e(this);
		e.exec();
	}
}

void MainWindow::ripple_delete() {
	if (sequence != NULL) panel_timeline->delete_selection(sequence->selections, true);
}

void MainWindow::editMenu_About_To_Be_Shown() {
	undo_action->setEnabled(undo_stack.canUndo());
	redo_action->setEnabled(undo_stack.canRedo());
}

void MainWindow::undo() {
	if (!panel_timeline->importing) { // workaround to prevent crash (and also users should never need to do this)
		undo_stack.undo();
		update_ui(true);
	}
}

void MainWindow::redo() {
	undo_stack.redo();
	update_ui(true);
}

void MainWindow::open_speed_dialog() {
	if (sequence != NULL) {
		SpeedDialog s(this);
		for (int i=0;i<sequence->clips.size();i++) {
			Clip* c = sequence->clips.at(i);
			if (c != NULL && panel_timeline->is_clip_selected(c, true)) {
				s.clips.append(c);
			}
		}
		if (s.clips.size() > 0) s.run();
	}
}

void MainWindow::cut() {
	if (sequence != NULL) {
		QDockWidget* focused_panel = get_focused_panel();
		if (panel_timeline == focused_panel) {
			panel_timeline->copy(true);
		} else if (panel_effect_controls == focused_panel) {
			panel_effect_controls->copy(true);
		}
	}
}

void MainWindow::copy() {
	if (sequence != NULL) {
		QDockWidget* focused_panel = get_focused_panel();
		if (panel_timeline == focused_panel) {
			panel_timeline->copy(false);
		} else if (panel_effect_controls == focused_panel) {
			panel_effect_controls->copy(false);
		}
	}
}

void MainWindow::paste() {
	QDockWidget* focused_panel = get_focused_panel();
	if ((panel_timeline == focused_panel || panel_effect_controls == focused_panel) && sequence != NULL) {
		panel_timeline->paste(false);
	}
}

void MainWindow::new_project() {
	if (can_close_project()) {
		panel_effect_controls->clear_effects(true);
		undo_stack.clear();
		project_url.clear();
		panel_project->new_project();
		updateTitle("");
		update_ui(false);
		panel_project->tree_view->update();
	}
}

void MainWindow::autorecover_interval() {
	if (!rendering && isWindowModified()) {
		panel_project->save_project(true);
		dout << "[INFO] Auto-recovery project saved";
	}
}

bool MainWindow::save_project_as() {
	QString fn = QFileDialog::getSaveFileName(this, "Save Project As...", "", OLIVE_FILE_FILTER);
	if (!fn.isEmpty()) {
		if (!fn.endsWith(".ove", Qt::CaseInsensitive)) {
			fn += ".ove";
		}
		updateTitle(fn);
		panel_project->save_project(false);
		return true;
	}
	return false;
}

bool MainWindow::save_project() {
	if (project_url.isEmpty()) {
		return save_project_as();
	} else {
		panel_project->save_project(false);
		return true;
	}
}

bool MainWindow::can_close_project() {
	if (isWindowModified()) {
		QMessageBox* m = new QMessageBox(
					QMessageBox::Question,
					"Unsaved Project",
					"This project has changed since it was last saved. Would you like to save it before closing?",
					QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,
					this
				);
		m->setWindowModality(Qt::WindowModal);
		int r = m->exec();
		if (r == QMessageBox::Yes) {
			return save_project();
		} else if (r == QMessageBox::Cancel) {
			return false;
		}
	}
	return true;
}

void MainWindow::setup_menus() {
	QMenuBar* menuBar = new QMenuBar(this);
	setMenuBar(menuBar);

	// INITIALIZE FILE MENU

	QMenu* file_menu = menuBar->addMenu("&File");
	connect(file_menu, SIGNAL(aboutToShow()), this, SLOT(fileMenu_About_To_Be_Shown()));

	QMenu* new_menu = file_menu->addMenu("&New");
	make_new_menu(new_menu);

	file_menu->addAction("&Open Project", this, SLOT(open_project()), QKeySequence("Ctrl+O"));

	clear_open_recent_action = new QAction("Clear Recent List");
	connect(clear_open_recent_action, SIGNAL(triggered()), panel_project, SLOT(clear_recent_projects()));

	open_recent = file_menu->addMenu("Open Recent");

	open_recent->addAction(clear_open_recent_action);

	file_menu->addAction("&Save Project", this, SLOT(save_project()), QKeySequence("Ctrl+S"));
	file_menu->addAction("Save Project &As", this, SLOT(save_project_as()), QKeySequence("Ctrl+Shift+S"));

	file_menu->addSeparator();

	file_menu->addAction("&Import...", panel_project, SLOT(import_dialog()), QKeySequence("Ctrl+I"));

	file_menu->addSeparator();

	file_menu->addAction("&Export...", this, SLOT(export_dialog()), QKeySequence("Ctrl+M"));

	file_menu->addSeparator();

	file_menu->addAction("E&xit", this, SLOT(close()));

	// INITIALIZE EDIT MENU

	QMenu* edit_menu = menuBar->addMenu("&Edit");
	connect(edit_menu, SIGNAL(aboutToShow()), this, SLOT(editMenu_About_To_Be_Shown()));

	undo_action = edit_menu->addAction("&Undo", this, SLOT(undo()), QKeySequence("Ctrl+Z"));
	redo_action = edit_menu->addAction("Redo", this, SLOT(redo()), QKeySequence("Ctrl+Shift+Z"));

	edit_menu->addSeparator();

	edit_menu->addAction("Cu&t", this, SLOT(cut()), QKeySequence("Ctrl+X"));
	edit_menu->addAction("Cop&y", this, SLOT(copy()), QKeySequence("Ctrl+C"));
	edit_menu->addAction("&Paste", this, SLOT(paste()), QKeySequence("Ctrl+V"));
	edit_menu->addAction("Paste Insert", this, SLOT(paste_insert()), QKeySequence("Ctrl+Shift+V"));
	edit_menu->addAction("Duplicate", this, SLOT(duplicate()), QKeySequence("Ctrl+D"));
	edit_menu->addAction("Delete", this, SLOT(delete_slot()), QKeySequence("Del"));
	edit_menu->addAction("Ripple Delete", this, SLOT(ripple_delete()), QKeySequence("Shift+Del"));
	edit_menu->addAction("Split", panel_timeline, SLOT(split_at_playhead()), QKeySequence("Ctrl+K"));

	edit_menu->addSeparator();

	edit_menu->addAction("Select &All", this, SLOT(select_all()), QKeySequence("Ctrl+A"));

	edit_menu->addAction("Deselect All", panel_timeline, SLOT(deselect()), QKeySequence("Ctrl+Shift+A"));

	edit_menu->addSeparator();

	edit_menu->addAction("Add Default Transition", this, SLOT(add_default_transition()), QKeySequence("Ctrl+Shift+D"));
	edit_menu->addAction("Link/Unlink", panel_timeline, SLOT(toggle_links()), QKeySequence("Ctrl+L"));
	edit_menu->addAction("Enable/Disable", this, SLOT(toggle_enable_clips()), QKeySequence("Shift+E"));
	edit_menu->addAction("Nest", this, SLOT(nest()));

	edit_menu->addSeparator();

	edit_menu->addAction("Ripple to In Point", this, SLOT(ripple_to_in_point()), QKeySequence("Q"));
	edit_menu->addAction("Ripple to Out Point", this, SLOT(ripple_to_out_point()), QKeySequence("W"));
	edit_menu->addAction("Edit to In Point", this, SLOT(edit_to_in_point()), QKeySequence("Ctrl+Alt+Q"));
	edit_menu->addAction("Edit to Out Point", this, SLOT(edit_to_out_point()), QKeySequence("Ctrl+Alt+W"));

	edit_menu->addSeparator();

	make_inout_menu(edit_menu);
	edit_menu->addAction("Delete In/Out Point", this, SLOT(delete_inout()), QKeySequence(";"));
	edit_menu->addAction("Ripple Delete In/Out Point", this, SLOT(ripple_delete_inout()), QKeySequence("'"));

	edit_menu->addSeparator();

	edit_menu->addAction("Set/Edit Marker", this, SLOT(set_marker()), QKeySequence("M"));

	// INITIALIZE VIEW MENU

	QMenu* view_menu = menuBar->addMenu("&View");
	connect(view_menu, SIGNAL(aboutToShow()), this, SLOT(viewMenu_About_To_Be_Shown()));

	view_menu->addAction("Zoom In", this, SLOT(zoom_in()), QKeySequence("="));
	view_menu->addAction("Zoom Out", this, SLOT(zoom_out()), QKeySequence("-"));
	view_menu->addAction("Increase Track Height", this, SLOT(zoom_in_tracks()), QKeySequence("Ctrl+="));
	view_menu->addAction("Decrease Track Height", this, SLOT(zoom_out_tracks()), QKeySequence("Ctrl+-"));

	show_all = view_menu->addAction("Toggle Show All", panel_timeline, SLOT(toggle_show_all()), QKeySequence("\\"));
	show_all->setCheckable(true);

	view_menu->addSeparator();

	track_lines = view_menu->addAction("Track Lines", this, SLOT(toggle_bool_action()));
	track_lines->setCheckable(true);
	track_lines->setData(reinterpret_cast<quintptr>(&config.show_track_lines));

	rectified_waveforms = view_menu->addAction("Rectified Waveforms", this, SLOT(toggle_bool_action()));
	rectified_waveforms->setCheckable(true);
	rectified_waveforms->setData(reinterpret_cast<quintptr>(&config.rectified_waveforms));

	view_menu->addSeparator();

	frames_action = view_menu->addAction("Frames", this, SLOT(set_timecode_view()));
	frames_action->setData(TIMECODE_FRAMES);
	frames_action->setCheckable(true);
	drop_frame_action = view_menu->addAction("Drop Frame", this, SLOT(set_timecode_view()));
	drop_frame_action->setData(TIMECODE_DROP);
	drop_frame_action->setCheckable(true);
	nondrop_frame_action = view_menu->addAction("Non-Drop Frame", this, SLOT(set_timecode_view()));
	nondrop_frame_action->setData(TIMECODE_NONDROP);
	nondrop_frame_action->setCheckable(true);
	milliseconds_action = view_menu->addAction("Milliseconds", this, SLOT(set_timecode_view()));
	milliseconds_action->setData(TIMECODE_MILLISECONDS);
	milliseconds_action->setCheckable(true);

	view_menu->addSeparator();

	QMenu* title_safe_area_menu = view_menu->addMenu("Title/Action Safe Area");

	title_safe_off = title_safe_area_menu->addAction("Off");
	title_safe_off->setCheckable(true);
	connect(title_safe_off, SIGNAL(triggered(bool)), this, SLOT(set_tsa_disable()));

	title_safe_default = title_safe_area_menu->addAction("Default");
	title_safe_default->setCheckable(true);
	connect(title_safe_default, SIGNAL(triggered(bool)), this, SLOT(set_tsa_default()));

	title_safe_43 = title_safe_area_menu->addAction("4:3");
	title_safe_43->setCheckable(true);
	connect(title_safe_43, SIGNAL(triggered(bool)), this, SLOT(set_tsa_43()));

	title_safe_169 = title_safe_area_menu->addAction("16:9");
	title_safe_169->setCheckable(true);
	connect(title_safe_169, SIGNAL(triggered(bool)), this, SLOT(set_tsa_169()));

	title_safe_custom = title_safe_area_menu->addAction("Custom");
	title_safe_custom->setCheckable(true);
	connect(title_safe_custom, SIGNAL(triggered(bool)), this, SLOT(set_tsa_custom()));

	view_menu->addSeparator();

	full_screen = view_menu->addAction("Full Screen", this, SLOT(toggle_full_screen()), QKeySequence("F11"));
	full_screen->setCheckable(true);

	// INITIALIZE PLAYBACK MENU

	QMenu* playback_menu = menuBar->addMenu("&Playback");

	playback_menu->addAction("Go to Start", this, SLOT(go_to_start()), QKeySequence("Home"));
	playback_menu->addAction("Previous Frame", this, SLOT(prev_frame()), QKeySequence("Left"));
	playback_menu->addAction("Play/Pause", this, SLOT(playpause()), QKeySequence("Space"));
	playback_menu->addAction("Next Frame", this, SLOT(next_frame()), QKeySequence("Right"));
	playback_menu->addAction("Go to End", this, SLOT(go_to_end()), QKeySequence("End"));
	playback_menu->addSeparator();
	playback_menu->addAction("Go to Previous Cut", this, SLOT(prev_cut()), QKeySequence("Up"));
	playback_menu->addAction("Go to Next Cut", this, SLOT(next_cut()), QKeySequence("Down"));
	playback_menu->addSeparator();
	playback_menu->addAction("Go to In Point", this, SLOT(go_to_in()), QKeySequence("Shift+I"));
	playback_menu->addAction("Go to Out Point", this, SLOT(go_to_out()), QKeySequence("Shift+O"));

	// INITIALIZE WINDOW MENU

	window_menu = menuBar->addMenu("&Window");
	connect(window_menu, SIGNAL(aboutToShow()), this, SLOT(windowMenu_About_To_Be_Shown()));

	QAction* window_project_action = window_menu->addAction("Project", this, SLOT(toggle_panel_visibility()));
	window_project_action->setCheckable(true);
	window_project_action->setData(reinterpret_cast<quintptr>(panel_project));

	QAction* window_effectcontrols_action = window_menu->addAction("Effect Controls", this, SLOT(toggle_panel_visibility()));
	window_effectcontrols_action->setCheckable(true);
	window_effectcontrols_action->setData(reinterpret_cast<quintptr>(panel_effect_controls));

	QAction* window_timeline_action = window_menu->addAction("Timeline", this, SLOT(toggle_panel_visibility()));
	window_timeline_action->setCheckable(true);
	window_timeline_action->setData(reinterpret_cast<quintptr>(panel_timeline));

	QAction* window_graph_editor_action = window_menu->addAction("Graph Editor", this, SLOT(toggle_panel_visibility()));
	window_graph_editor_action->setCheckable(true);
	window_graph_editor_action->setData(reinterpret_cast<quintptr>(panel_graph_editor));

	QAction* window_footageviewer_action = window_menu->addAction("Footage Viewer", this, SLOT(toggle_panel_visibility()));
	window_footageviewer_action->setCheckable(true);
	window_footageviewer_action->setData(reinterpret_cast<quintptr>(panel_footage_viewer));

	QAction* window_sequenceviewer_action = window_menu->addAction("Sequence Viewer", this, SLOT(toggle_panel_visibility()));
	window_sequenceviewer_action->setCheckable(true);
	window_sequenceviewer_action->setData(reinterpret_cast<quintptr>(panel_sequence_viewer));

	window_menu->addSeparator();

	window_menu->addAction("Reset to Default Layout", this, SLOT(reset_layout()));

	// INITIALIZE TOOLS MENU

	QMenu* tools_menu = menuBar->addMenu("&Tools");
	connect(tools_menu, SIGNAL(aboutToShow()), this, SLOT(toolMenu_About_To_Be_Shown()));

	pointer_tool_action = tools_menu->addAction("Pointer Tool", this, SLOT(menu_click_button()), QKeySequence("V"));
	pointer_tool_action->setCheckable(true);
	pointer_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline->toolArrowButton));

	edit_tool_action = tools_menu->addAction("Edit Tool", this, SLOT(menu_click_button()), QKeySequence("X"));
	edit_tool_action->setCheckable(true);
	edit_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline->toolEditButton));

	ripple_tool_action = tools_menu->addAction("Ripple Tool", this, SLOT(menu_click_button()), QKeySequence("B"));
	ripple_tool_action->setCheckable(true);
	ripple_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline->toolRippleButton));

	razor_tool_action = tools_menu->addAction("Razor Tool", this, SLOT(menu_click_button()), QKeySequence("C"));
	razor_tool_action->setCheckable(true);
	razor_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline->toolRazorButton));

	slip_tool_action = tools_menu->addAction("Slip Tool", this, SLOT(menu_click_button()), QKeySequence("Y"));
	slip_tool_action->setCheckable(true);
	slip_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline->toolSlipButton));

	slide_tool_action = tools_menu->addAction("Slide Tool", this, SLOT(menu_click_button()), QKeySequence("U"));
	slide_tool_action->setCheckable(true);
	slide_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline->toolSlideButton));

	hand_tool_action = tools_menu->addAction("Hand Tool", this, SLOT(menu_click_button()), QKeySequence("H"));
	hand_tool_action->setCheckable(true);
	hand_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline->toolHandButton));

	transition_tool_action = tools_menu->addAction("Transition Tool", this, SLOT(menu_click_button()), QKeySequence("T"));
	transition_tool_action->setCheckable(true);
	transition_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline->toolTransitionButton));

	tools_menu->addSeparator();

	snap_toggle = tools_menu->addAction("Enable Snapping", this, SLOT(menu_click_button()), QKeySequence("S"));
	snap_toggle->setCheckable(true);
	snap_toggle->setData(reinterpret_cast<quintptr>(panel_timeline->snappingButton));

	tools_menu->addSeparator();

	selecting_also_seeks = tools_menu->addAction("Selecting Also Seeks", this, SLOT(toggle_bool_action()));
	selecting_also_seeks->setCheckable(true);
	selecting_also_seeks->setData(reinterpret_cast<quintptr>(&config.select_also_seeks));

	edit_tool_also_seeks = tools_menu->addAction("Edit Tool Also Seeks", this, SLOT(toggle_bool_action()));
	edit_tool_also_seeks->setCheckable(true);
	edit_tool_also_seeks->setData(reinterpret_cast<quintptr>(&config.edit_tool_also_seeks));

	edit_tool_selects_links = tools_menu->addAction("Edit Tool Selects Links", this, SLOT(toggle_bool_action()));
	edit_tool_selects_links->setCheckable(true);
	edit_tool_selects_links->setData(reinterpret_cast<quintptr>(&config.edit_tool_selects_links));

	seek_to_end_of_pastes = tools_menu->addAction("Seek to the End of Pastes", this, SLOT(toggle_bool_action()));
	seek_to_end_of_pastes->setCheckable(true);
	seek_to_end_of_pastes->setData(reinterpret_cast<quintptr>(&config.paste_seeks));

	scroll_wheel_zooms = tools_menu->addAction("Scroll Wheel Zooms", this, SLOT(toggle_bool_action()));
	scroll_wheel_zooms->setCheckable(true);
	scroll_wheel_zooms->setData(reinterpret_cast<quintptr>(&config.scroll_zooms));

	enable_drag_files_to_timeline = tools_menu->addAction("Enable Drag Files to Timeline", this, SLOT(toggle_bool_action()));
	enable_drag_files_to_timeline->setCheckable(true);
	enable_drag_files_to_timeline->setData(reinterpret_cast<quintptr>(&config.enable_drag_files_to_timeline));

	autoscale_by_default = tools_menu->addAction("Auto-Scale By Default", this, SLOT(toggle_bool_action()));
	autoscale_by_default->setCheckable(true);
	autoscale_by_default->setData(reinterpret_cast<quintptr>(&config.autoscale_by_default));

	enable_seek_to_import = tools_menu->addAction("Enable Seek to Import", this, SLOT(toggle_bool_action()));
	enable_seek_to_import->setCheckable(true);
	enable_seek_to_import->setData(reinterpret_cast<quintptr>(&config.enable_seek_to_import));

	enable_audio_scrubbing = tools_menu->addAction("Audio Scrubbing", this, SLOT(toggle_bool_action()));
	enable_audio_scrubbing->setCheckable(true);
	enable_audio_scrubbing->setData(reinterpret_cast<quintptr>(&config.enable_audio_scrubbing));

	enable_drop_on_media_to_replace = tools_menu->addAction("Enable Drop on Media to Replace", this, SLOT(toggle_bool_action()));
	enable_drop_on_media_to_replace->setCheckable(true);
	enable_drop_on_media_to_replace->setData(reinterpret_cast<quintptr>(&config.drop_on_media_to_replace));

	enable_hover_focus = tools_menu->addAction("Enable Hover Focus", this, SLOT(toggle_bool_action()));
	enable_hover_focus->setCheckable(true);
	enable_hover_focus->setData(reinterpret_cast<quintptr>(&config.hover_focus));

	set_name_and_marker = tools_menu->addAction("Ask For Name When Setting Marker", this, SLOT(toggle_bool_action()));
	set_name_and_marker->setCheckable(true);
	set_name_and_marker->setData(reinterpret_cast<quintptr>(&config.set_name_with_marker));

	loop_action = tools_menu->addAction("Loop", this, SLOT(toggle_bool_action()));
	loop_action->setCheckable(true);
	loop_action->setData(reinterpret_cast<quintptr>(&config.loop));

	pause_at_out_point_action = tools_menu->addAction("Pause At Out Point", this, SLOT(toggle_bool_action()));
	pause_at_out_point_action->setCheckable(true);
	pause_at_out_point_action->setData(reinterpret_cast<quintptr>(&config.pause_at_out_point));

	tools_menu->addSeparator();

	no_autoscroll = tools_menu->addAction("No Auto-Scroll", this, SLOT(set_autoscroll()));
	no_autoscroll->setData(AUTOSCROLL_NO_SCROLL);
	no_autoscroll->setCheckable(true);

	page_autoscroll = tools_menu->addAction("Page Auto-Scroll", this, SLOT(set_autoscroll()));
	page_autoscroll->setData(AUTOSCROLL_PAGE_SCROLL);
	page_autoscroll->setCheckable(true);

	smooth_autoscroll = tools_menu->addAction("Smooth Auto-Scroll", this, SLOT(set_autoscroll()));
	smooth_autoscroll->setData(AUTOSCROLL_SMOOTH_SCROLL);
	smooth_autoscroll->setCheckable(true);

	tools_menu->addSeparator();

	tools_menu->addAction("Preferences", this, SLOT(preferences()), QKeySequence("Ctrl+."));

#ifdef QT_DEBUG
	tools_menu->addAction("Clear Undo", this, SLOT(clear_undo_stack()));
#endif

	// INITIALIZE HELP MENU

	QMenu* help_menu = menuBar->addMenu("&Help");

	help_menu->addAction("A&ction Search", this, SLOT(show_action_search()), QKeySequence("/"));

	help_menu->addSeparator();

	help_menu->addAction("&About...", this, SLOT(show_about()));

	load_shortcuts(get_config_path() + "/shortcuts", true);
}

void MainWindow::set_bool_action_checked(QAction *a) {
	if (!a->data().isNull()) {
		bool* variable = reinterpret_cast<bool*>(a->data().value<quintptr>());
		a->setChecked(*variable);
	}
}

void MainWindow::set_int_action_checked(QAction *a, const int& i) {
	if (!a->data().isNull()) {
		a->setChecked(a->data() == i);
	}
}

void MainWindow::set_button_action_checked(QAction *a) {
	a->setChecked(reinterpret_cast<QPushButton*>(a->data().value<quintptr>())->isChecked());
}

void MainWindow::updateTitle(const QString& url) {
	project_url = url;
	setWindowTitle(appName + " - " + ((project_url.isEmpty()) ? "<untitled>" : project_url) + "[*]");
}

void MainWindow::closeEvent(QCloseEvent *e) {
	if (can_close_project()) {
		panel_effect_controls->clear_effects(true);

		set_sequence(NULL);

		panel_footage_viewer->set_main_sequence();

		QString data_dir = get_data_path();
		QString config_dir = get_config_path();
		if (!data_dir.isEmpty() && !autorecovery_filename.isEmpty()) {
			if (QFile::exists(autorecovery_filename)) {
				QFile::rename(autorecovery_filename, autorecovery_filename + "." + QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmss"));
			}
		}
		if (!config_dir.isEmpty() && !config_fn.isEmpty()) {
			// save settings
			config.save(config_fn);

			// save panel layout
			QFile panel_config(config_dir + "/layout");
			if (panel_config.open(QFile::WriteOnly)) {
				panel_config.write(saveState(0));
				panel_config.close();
			} else {
				dout << "[ERROR] Failed to save layout";
			}

			save_shortcuts(config_dir + "/shortcuts");
		}

		stop_audio();

		e->accept();
	} else {
		e->ignore();
	}
}

void MainWindow::paintEvent(QPaintEvent *event) {
	QMainWindow::paintEvent(event);
	if (enable_launch_with_project) {
		QTimer::singleShot(10, this, SLOT(load_with_launch()));
		enable_launch_with_project = false;
	}
#ifndef QT_DEBUG
	if (!demoNoticeShown) {
		DemoNotice* d = new DemoNotice(this);
		d->open();
		demoNoticeShown = true;
	}
#endif
}

void MainWindow::clear_undo_stack() {
	undo_stack.clear();
}

void MainWindow::open_project() {
	QString fn = QFileDialog::getOpenFileName(this, "Open Project...", "", OLIVE_FILE_FILTER);
	if (!fn.isEmpty() && can_close_project()) {
		open_project_worker(fn, false);
	}
}

void MainWindow::open_project_worker(const QString& fn, bool autorecovery) {
	updateTitle(fn);
	panel_project->load_project(autorecovery);
	undo_stack.clear();
}

void MainWindow::load_with_launch() {
	open_project_worker(project_url, false);
}

void MainWindow::show_action_search() {
	ActionSearch as(this);
	as.exec();
}

void MainWindow::reset_layout() {
	setup_layout(true);
}

void MainWindow::go_to_in() {
	if (panel_timeline->focused()
			|| panel_sequence_viewer->is_focused()
			|| panel_effect_controls->keyframe_focus()
			|| panel_graph_editor->view_is_focused()) {
		panel_sequence_viewer->go_to_in();
	} else if (panel_footage_viewer->is_focused()) {
		panel_footage_viewer->go_to_in();
	}
}

void MainWindow::go_to_out() {
	if (panel_timeline->focused()
			|| panel_sequence_viewer->is_focused()
			|| panel_effect_controls->keyframe_focus()
			|| panel_graph_editor->view_is_focused()) {
		panel_sequence_viewer->go_to_out();
	} else if (panel_footage_viewer->is_focused()) {
		panel_footage_viewer->go_to_out();
	}
}

void MainWindow::go_to_start() {
	if (panel_timeline->focused()
			|| panel_sequence_viewer->is_focused()
			|| panel_effect_controls->keyframe_focus()
			|| panel_graph_editor->view_is_focused()) {
		panel_sequence_viewer->go_to_start();
	} else if (panel_footage_viewer->is_focused()) {
		panel_footage_viewer->go_to_start();
	}
}

void MainWindow::prev_frame() {
	if (panel_timeline->focused()
			|| panel_sequence_viewer->is_focused()
			|| panel_effect_controls->keyframe_focus()
			|| panel_graph_editor->view_is_focused()) {
		panel_sequence_viewer->previous_frame();
	} else if (panel_footage_viewer->is_focused()) {
		panel_footage_viewer->previous_frame();
	}
}

void MainWindow::next_frame() {
	if (panel_timeline->focused()
			|| panel_sequence_viewer->is_focused()
			|| panel_effect_controls->keyframe_focus()
			|| panel_graph_editor->view_is_focused()) {
		panel_sequence_viewer->next_frame();
	} else if (panel_footage_viewer->is_focused()) {
		panel_footage_viewer->next_frame();
	}
}

void MainWindow::go_to_end() {
	if (panel_timeline->focused()
			|| panel_sequence_viewer->is_focused()
			|| panel_effect_controls->keyframe_focus()
			|| panel_graph_editor->view_is_focused()) {
		panel_sequence_viewer->go_to_end();
	} else if (panel_footage_viewer->is_focused()) {
		panel_footage_viewer->go_to_end();
	}
}

void MainWindow::playpause() {
	if (panel_timeline->focused()
			|| panel_sequence_viewer->is_focused()
			|| panel_effect_controls->keyframe_focus()
			|| panel_graph_editor->view_is_focused()) {
		panel_sequence_viewer->toggle_play();
	} else if (panel_footage_viewer->is_focused()) {
		panel_footage_viewer->toggle_play();
	}
}

void MainWindow::prev_cut() {
	if (sequence != NULL && (panel_timeline->focused() || panel_sequence_viewer->is_focused())) {
		panel_timeline->previous_cut();
	}
}

void MainWindow::next_cut() {
	if (sequence != NULL && (panel_timeline->focused() || panel_sequence_viewer->is_focused())) {
		panel_timeline->next_cut();
	}
}

void MainWindow::preferences()
{
	PreferencesDialog pd(this);
	pd.setup_kbd_shortcuts(menuBar());
	pd.exec();
}

void MainWindow::zoom_in_tracks() {
	panel_timeline->increase_track_height();
}

void MainWindow::zoom_out_tracks() {
	panel_timeline->decrease_track_height();
}

void MainWindow::windowMenu_About_To_Be_Shown() {
	QList<QAction*> window_actions = window_menu->actions();
	for (int i=0;i<window_actions.size();i++) {
		QAction* a = window_actions.at(i);
		if (!a->data().isNull()) {
			a->setChecked(reinterpret_cast<QDockWidget*>(a->data().value<quintptr>())->isVisible());
		}
	}
}

void MainWindow::viewMenu_About_To_Be_Shown() {
	set_bool_action_checked(track_lines);

	set_int_action_checked(frames_action, config.timecode_view);
	set_int_action_checked(drop_frame_action, config.timecode_view);
	set_int_action_checked(nondrop_frame_action, config.timecode_view);
	set_int_action_checked(milliseconds_action, config.timecode_view);

	title_safe_off->setChecked(!config.show_title_safe_area);
	title_safe_default->setChecked(config.show_title_safe_area && !config.use_custom_title_safe_ratio);
	title_safe_43->setChecked(config.show_title_safe_area && config.use_custom_title_safe_ratio && config.custom_title_safe_ratio == 4.0/3.0);
	title_safe_169->setChecked(config.show_title_safe_area && config.use_custom_title_safe_ratio && config.custom_title_safe_ratio == 16.0/9.0);
	title_safe_custom->setChecked(config.show_title_safe_area && config.use_custom_title_safe_ratio && !title_safe_43->isChecked() && !title_safe_169->isChecked());

	full_screen->setChecked(windowState() == Qt::WindowFullScreen);

	show_all->setChecked(panel_timeline->showing_all);
}

void MainWindow::toolMenu_About_To_Be_Shown() {
	set_button_action_checked(pointer_tool_action);
	set_button_action_checked(edit_tool_action);
	set_button_action_checked(ripple_tool_action);
	set_button_action_checked(razor_tool_action);
	set_button_action_checked(slip_tool_action);
	set_button_action_checked(slide_tool_action);
	set_button_action_checked(hand_tool_action);
	set_button_action_checked(transition_tool_action);
	set_button_action_checked(snap_toggle);

	set_bool_action_checked(selecting_also_seeks);
	set_bool_action_checked(edit_tool_also_seeks);
	set_bool_action_checked(edit_tool_selects_links);
	set_bool_action_checked(seek_to_end_of_pastes);
	set_bool_action_checked(scroll_wheel_zooms);
	set_bool_action_checked(rectified_waveforms);
	set_bool_action_checked(enable_drag_files_to_timeline);
	set_bool_action_checked(autoscale_by_default);
	set_bool_action_checked(enable_seek_to_import);
	set_bool_action_checked(enable_audio_scrubbing);
	set_bool_action_checked(enable_drop_on_media_to_replace);
	set_bool_action_checked(enable_hover_focus);
	set_bool_action_checked(set_name_and_marker);
	set_bool_action_checked(loop_action);
	set_bool_action_checked(pause_at_out_point_action);

	set_int_action_checked(no_autoscroll, config.autoscroll);
	set_int_action_checked(page_autoscroll, config.autoscroll);
	set_int_action_checked(smooth_autoscroll, config.autoscroll);
}

void MainWindow::duplicate() {
	if (panel_project->is_focused()) {
		panel_project->duplicate_selected();
	}
}

void MainWindow::add_default_transition() {
	if (panel_timeline->focused()) panel_timeline->add_transition();
}

void MainWindow::new_folder() {
	Media* m = panel_project->new_folder(0);
	undo_stack.push(new AddMediaCommand(m, panel_project->get_selected_folder()));

	QModelIndex index = project_model.create_index(m->row(), 0, m);
	switch (config.project_view_type) {
	case PROJECT_VIEW_TREE:
		panel_project->tree_view->edit(panel_project->sorter->mapFromSource(index));
		break;
	case PROJECT_VIEW_ICON:
		panel_project->icon_view->edit(panel_project->sorter->mapFromSource(index));
		break;
	}
}

void MainWindow::fileMenu_About_To_Be_Shown() {
	if (recent_projects.size() > 0) {
		open_recent->clear();
		open_recent->setEnabled(true);
		for (int i=0;i<recent_projects.size();i++) {
			QAction* action = open_recent->addAction(recent_projects.at(i));
			action->setProperty("keyignore", true);
			action->setData(i);
			connect(action, SIGNAL(triggered()), this, SLOT(load_recent_project()));
		}
		open_recent->addSeparator();

		open_recent->addAction(clear_open_recent_action);
	} else {
		open_recent->setEnabled(false);
	}
}

void MainWindow::fileMenu_About_To_Hide() {
//	open_recent->clear();
}

void MainWindow::load_recent_project() {
	int index = static_cast<QAction*>(sender())->data().toInt();
	QString recent_url = recent_projects.at(index);
	if (!QFile::exists(recent_url)) {
		if (QMessageBox::question(this, "Missing recent project", "The project '" + recent_url + "' no longer exists. Would you like to remove it from the recent projects list?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
			recent_projects.removeAt(index);
			panel_project->save_recent_projects();
		}
	} else if (can_close_project()) {
		open_project_worker(recent_url, false);
	}
}

void MainWindow::ripple_to_in_point() {
	if (panel_timeline->focused()) panel_timeline->ripple_to_in_point(true, true);
}

void MainWindow::ripple_to_out_point() {
	if (panel_timeline->focused()) panel_timeline->ripple_to_in_point(false, true);
}

void MainWindow::set_in_point() {
	if (panel_timeline->focused() || panel_sequence_viewer->is_focused()) {
		panel_sequence_viewer->set_in_point();
	} else if (panel_footage_viewer->is_focused()) {
		panel_footage_viewer->set_in_point();
	}
}

void MainWindow::set_out_point() {
	if (panel_timeline->focused() || panel_sequence_viewer->is_focused()) {
		panel_sequence_viewer->set_out_point();
	} else if (panel_footage_viewer->is_focused()) {
		panel_footage_viewer->set_out_point();
	}
}

void MainWindow::clear_in() {
	if (panel_timeline->focused() || panel_sequence_viewer->is_focused()) {
		panel_sequence_viewer->clear_in();
	} else if (panel_footage_viewer->is_focused()) {
		panel_footage_viewer->clear_in();
	}
}

void MainWindow::clear_out() {
	if (panel_timeline->focused() || panel_sequence_viewer->is_focused()) {
		panel_sequence_viewer->clear_out();
	} else if (panel_footage_viewer->is_focused()) {
		panel_footage_viewer->clear_out();
	}
}

void MainWindow::clear_inout() {
	if (panel_timeline->focused() || panel_sequence_viewer->is_focused()) {
		panel_sequence_viewer->clear_inout_point();
	} else if (panel_footage_viewer->is_focused()) {
		panel_footage_viewer->clear_inout_point();
	}
}

void MainWindow::toggle_full_screen() {
	if (windowState() == Qt::WindowFullScreen) {
		setWindowState(Qt::WindowNoState); // seems to be necessary for it to return to Maximized correctly on Linux
		setWindowState(Qt::WindowMaximized);
	} else {
		setWindowState(Qt::WindowFullScreen);
	}
}

void MainWindow::delete_inout() {
	if (panel_timeline->focused()) {
		panel_timeline->delete_in_out(false);
	}
}

void MainWindow::ripple_delete_inout()
{
	if (panel_timeline->focused()) {
		panel_timeline->delete_in_out(true);
	}
}

void MainWindow::enable_inout() {
	if (panel_timeline->focused() || panel_sequence_viewer->is_focused()) {
		panel_sequence_viewer->toggle_enable_inout();
	} else if (panel_footage_viewer->is_focused()) {
		panel_footage_viewer->toggle_enable_inout();
	}
}

void MainWindow::set_tsa_default() {
	config.show_title_safe_area = true;
	config.use_custom_title_safe_ratio = false;
	panel_sequence_viewer->viewer_widget->update();
}

void MainWindow::set_tsa_disable() {
	config.show_title_safe_area = false;
	panel_sequence_viewer->viewer_widget->update();
}

void MainWindow::set_tsa_43() {
	config.show_title_safe_area = true;
	config.use_custom_title_safe_ratio = true;
	config.custom_title_safe_ratio = 4.0/3.0;
	panel_sequence_viewer->viewer_widget->update();
}

void MainWindow::set_tsa_169() {
	config.show_title_safe_area = true;
	config.use_custom_title_safe_ratio = true;
	config.custom_title_safe_ratio = 16.0/9.0;
	panel_sequence_viewer->viewer_widget->update();
}

void MainWindow::set_tsa_custom() {
	QString input;
	bool invalid = false;
	QRegExp arTest("[0-9.]+:[0-9.]+");

	do {
		if (invalid) {
			QMessageBox::critical(this, "Invalid aspect ratio", "The aspect ratio '" + input + "' is invalid. Please try again.");
		}

		input = QInputDialog::getText(this, "Enter custom aspect ratio", "Enter the aspect ratio to use for the title/action safe area (e.g. 16:9):");
		invalid = !arTest.exactMatch(input) && !input.isEmpty();
	} while (invalid);

	if (!input.isEmpty()) {
		QStringList inputList = input.split(':');

		config.show_title_safe_area = true;
		config.use_custom_title_safe_ratio = true;
		config.custom_title_safe_ratio = inputList.at(0).toDouble()/inputList.at(1).toDouble();
		panel_sequence_viewer->viewer_widget->update();
	}
}

void MainWindow::set_marker() {
	if (sequence != NULL) panel_timeline->set_marker();
}

void MainWindow::toggle_enable_clips() {
	if (sequence != NULL) {
		ComboAction* ca = new ComboAction();
		bool push_undo = false;
		for (int i=0;i<sequence->clips.size();i++) {
			Clip* c = sequence->clips.at(i);
			if (c != NULL && panel_timeline->is_clip_selected(c, true)) {
				ca->append(new SetEnableCommand(c, !c->enabled));
				push_undo = true;
			}
		}
		if (push_undo) {
			undo_stack.push(ca);
			update_ui(true);
		} else {
			delete ca;
		}
	}
}

void MainWindow::edit_to_in_point() {
	if (panel_timeline->focused()) panel_timeline->ripple_to_in_point(true, false);
}

void MainWindow::edit_to_out_point() {
	if (panel_timeline->focused()) panel_timeline->ripple_to_in_point(false, false);
}

void MainWindow::nest() {
	if (sequence != NULL) {
		QVector<int> selected_clips;
		long earliest_point = LONG_MAX;

		// get selected clips
		for (int i=0;i<sequence->clips.size();i++) {
			Clip* c = sequence->clips.at(i);
			if (c != NULL && panel_timeline->is_clip_selected(c, true)) {
				selected_clips.append(i);
				earliest_point = qMin(c->timeline_in, earliest_point);
			}
		}

		// nest them
		if (!selected_clips.isEmpty()) {
			ComboAction* ca = new ComboAction();

			Sequence* s = new Sequence();

			// create "nest" sequence
			s->name = panel_project->get_next_sequence_name("Nested Sequence");
			s->width = sequence->width;
			s->height = sequence->height;
			s->frame_rate = sequence->frame_rate;
			s->audio_frequency = sequence->audio_frequency;
			s->audio_layout = sequence->audio_layout;

			// copy all selected clips to the nest
			for (int i=0;i<selected_clips.size();i++) {
				// delete clip from old sequence
				ca->append(new DeleteClipAction(sequence, selected_clips.at(i)));

				// copy to new
				Clip* copy = sequence->clips.at(selected_clips.at(i))->copy(s);
				copy->timeline_in -= earliest_point;
				copy->timeline_out -= earliest_point;
				s->clips.append(copy);
			}

			// add sequence to project
			Media* m = panel_project->new_sequence(ca, s, false, NULL);

			// add nested sequence to active sequence
			QVector<Media*> media_list;
			media_list.append(m);
			panel_timeline->create_ghosts_from_media(sequence, earliest_point, media_list);
			panel_timeline->add_clips_from_ghosts(ca, sequence);

			panel_effect_controls->clear_effects(true);
			sequence->selections.clear();

			undo_stack.push(ca);

			update_ui(true);
		}
	}
}

void MainWindow::paste_insert() {
	if (panel_timeline->focused() && sequence != NULL) {
		panel_timeline->paste(true);
	}
}

void MainWindow::toggle_bool_action() {
	QAction* action = static_cast<QAction*>(sender());
	bool* variable = reinterpret_cast<bool*>(action->data().value<quintptr>());
	*variable = !(*variable);
	update_ui(false);
}

void MainWindow::set_autoscroll() {
	QAction* action = static_cast<QAction*>(sender());
	config.autoscroll = action->data().toInt();
}

void MainWindow::menu_click_button() {
	if (panel_timeline->focused()
			|| panel_effect_controls->keyframe_focus()
			|| panel_footage_viewer->is_focused()
			|| panel_sequence_viewer->is_focused())
		reinterpret_cast<QPushButton*>(static_cast<QAction*>(sender())->data().value<quintptr>())->click();
}

void MainWindow::toggle_panel_visibility() {
	QAction* action = static_cast<QAction*>(sender());
	QDockWidget* w = reinterpret_cast<QDockWidget*>(action->data().value<quintptr>());
	w->setVisible(!w->isVisible());
}

void MainWindow::set_timecode_view() {
	QAction* action = static_cast<QAction*>(sender());
	config.timecode_view = action->data().toInt();
	update_ui(false);
}
