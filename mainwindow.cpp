/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "mainwindow.h"

#include "oliveglobal.h"

#include "ui/menuhelper.h"

#include "project/projectelements.h"

#include "io/config.h"
#include "io/path.h"
#include "io/proxygenerator.h"

#include "project/projectfilter.h"

#include "ui/sourcetable.h"
#include "ui/viewerwidget.h"
#include "ui/sourceiconview.h"
#include "ui/timelineheader.h"
#include "ui/cursors.h"
#include "ui/focusfilter.h"

#include "panels/panels.h"

#include "dialogs/debugdialog.h"

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
#include <QStatusBar>
#include <QMenu>
#include <QMenuBar>
#include <QLayout>
#include <QApplication>
#include <QPushButton>
#include <QTranslator>

Q_DECLARE_METATYPE(ClipPtr);

MainWindow* olive::MainWindow;

#define DEFAULT_CSS "QPushButton::checked { background: rgb(25, 25, 25); }"

void MainWindow::setup_layout(bool reset) {
  panel_project->show();
  panel_effect_controls->show();
  panel_footage_viewer->show();
  panel_sequence_viewer->show();
  panel_timeline->show();
  panel_graph_editor->hide();

  addDockWidget(Qt::TopDockWidgetArea, panel_project);
  addDockWidget(Qt::TopDockWidgetArea, panel_graph_editor);
  addDockWidget(Qt::TopDockWidgetArea, panel_footage_viewer);
  tabifyDockWidget(panel_footage_viewer, panel_effect_controls);
  panel_footage_viewer->raise();
  addDockWidget(Qt::TopDockWidgetArea, panel_sequence_viewer);
  addDockWidget(Qt::BottomDockWidgetArea, panel_timeline);

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

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  first_show(true)
{
  qRegisterMetaType<ClipPtr>();

  init_custom_cursors();

  open_debug_file();

  olive::DebugDialog = new DebugDialog(this);

  olive::MainWindow = this;

  // set up style?

  qApp->setStyle(QStyleFactory::create("Fusion"));
  setStyleSheet(DEFAULT_CSS);

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
      if (deleted_ars > 0) qInfo() << "Deleted" << deleted_ars << "autorecovery" << ((deleted_ars == 1) ? "file that was" : "files that were") << "older than 7 days";

      // delete previews older than 30 days
      QDir preview_dir = QDir(dir.filePath("previews"));
      if (preview_dir.exists()) {
        deleted_ars = 0;
        QStringList old_prevs = preview_dir.entryList(QDir::Files);
        for (int i=0;i<old_prevs.size();i++) {
          QString file_name = preview_dir.filePath(old_prevs.at(i));
          qint64 file_time = QFileInfo(file_name).lastRead().toMSecsSinceEpoch();
          if (file_time < a_month_ago) {
            if (QFile(file_name).remove()) deleted_ars++;
          }
        }
        if (deleted_ars > 0) qInfo() << "Deleted" << deleted_ars << "preview" << ((deleted_ars == 1) ? "file that was" : "files that were") << "last read over 30 days ago";
      }

      // search for open recents list
      QFile f(olive::Global->get_recent_project_list_file());
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
    QString config_fn = config_dir.filePath("config.xml");
    if (QFileInfo::exists(config_fn)) {
      olive::CurrentConfig.load(config_fn);

      if (!olive::CurrentConfig.css_path.isEmpty()) {
        load_css_from_file(olive::CurrentConfig.css_path);
      }
    }
  }

  // load preferred language from file
  olive::Global->load_translation_from_config();

  alloc_panels(this);

  QStatusBar* statusBar = new QStatusBar(this);
  statusBar->showMessage(tr("Welcome to %1").arg(olive::AppName));
  setStatusBar(statusBar);

  // populate menu bars
  setup_menus();

  olive::Global->check_for_autorecovery_file();

  // set up panel layout
  setup_layout(false);

  // set up output audio device
  init_audio();

  // start omnipotent proxy generator process
  proxy_generator.start();

  // set default window title
  updateTitle();
}

MainWindow::~MainWindow() {
  free_panels();
  close_debug_file();
}

void kbd_shortcut_processor(QByteArray& file, QMenu* menu, bool save, bool first) {
  QList<QAction*> actions = menu->actions();
  for (int i=0;i<actions.size();i++) {
    QAction* a = actions.at(i);
    if (a->menu() != nullptr) {
      kbd_shortcut_processor(file, a->menu(), save, first);
    } else if (!a->isSeparator()) {
      if (save) {
        // saving custom shortcuts
        if (!a->property("default").isNull()) {
          QKeySequence defks(a->property("default").toString());
          if (a->shortcut() != defks) {
            // custom shortcut
            if (!file.isEmpty()) file.append('\n');
            file.append(a->property("id").toString());
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
        if (!a->property("id").isNull()) {
          QString comp_str = a->property("id").toString();
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
        a->setShortcutContext(Qt::ApplicationShortcut);
      }
    }
  }
}

void MainWindow::load_shortcuts(const QString& fn) {
  QByteArray shortcut_bytes;
  QFile shortcut_path(fn);
  if (shortcut_path.exists() && shortcut_path.open(QFile::ReadOnly)) {
    shortcut_bytes = shortcut_path.readAll();
    shortcut_path.close();
  }
  QList<QAction*> menus = menuBar()->actions();
  for (int i=0;i<menus.size();i++) {
    QMenu* menu = menus.at(i)->menu();
    kbd_shortcut_processor(shortcut_bytes, menu, false, true);
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
    qCritical() << "Failed to save shortcut file";
  }
}

void MainWindow::load_css_from_file(const QString &fn) {
  QFile css_file(fn);
  if (css_file.exists() && css_file.open(QFile::ReadOnly)) {
    setStyleSheet(css_file.readAll());
    css_file.close();
  } else {
    // set default stylesheet
    setStyleSheet(DEFAULT_CSS);
  }
}

void MainWindow::editMenu_About_To_Be_Shown() {
  undo_action->setEnabled(olive::UndoStack.canUndo());
  redo_action->setEnabled(olive::UndoStack.canRedo());
}

void MainWindow::setup_menus() {
  QMenuBar* menuBar = new QMenuBar(this);
  setMenuBar(menuBar);

  // INITIALIZE FILE MENU

  QMenu* file_menu = menuBar->addMenu(tr("&File"));
  connect(file_menu, SIGNAL(aboutToShow()), this, SLOT(fileMenu_About_To_Be_Shown()));

  QMenu* new_menu = file_menu->addMenu(tr("&New"));
  olive::MenuHelper.make_new_menu(new_menu);

  file_menu->addAction(tr("&Open Project"), olive::Global.get(), SLOT(open_project()), QKeySequence("Ctrl+O"))->setProperty("id", "openproj");

  clear_open_recent_action = new QAction(tr("Clear Recent List"), menuBar);
  clear_open_recent_action->setProperty("id", "clearopenrecent");
  connect(clear_open_recent_action, SIGNAL(triggered()), panel_project, SLOT(clear_recent_projects()));

  open_recent = file_menu->addMenu(tr("Open Recent"));

  open_recent->addAction(clear_open_recent_action);

  file_menu->addAction(tr("&Save Project"), olive::Global.get(), SLOT(save_project()), QKeySequence("Ctrl+S"))->setProperty("id", "saveproj");
  file_menu->addAction(tr("Save Project &As"), olive::Global.get(), SLOT(save_project_as()), QKeySequence("Ctrl+Shift+S"))->setProperty("id", "saveprojas");

  file_menu->addSeparator();

  file_menu->addAction(tr("&Import..."), panel_project, SLOT(import_dialog()), QKeySequence("Ctrl+I"))->setProperty("id", "import");

  file_menu->addSeparator();

  file_menu->addAction(tr("&Export..."), olive::Global.get(), SLOT(open_export_dialog()), QKeySequence("Ctrl+M"))->setProperty("id", "export");

  file_menu->addSeparator();

  file_menu->addAction(tr("E&xit"), this, SLOT(close()))->setProperty("id", "exit");

  // INITIALIZE EDIT MENU

  QMenu* edit_menu = menuBar->addMenu(tr("&Edit"));
  connect(edit_menu, SIGNAL(aboutToShow()), this, SLOT(editMenu_About_To_Be_Shown()));

  undo_action = edit_menu->addAction(tr("&Undo"), olive::Global.get(), SLOT(undo()), QKeySequence("Ctrl+Z"));
  undo_action->setProperty("id", "undo");
  redo_action = edit_menu->addAction(tr("Redo"), olive::Global.get(), SLOT(redo()), QKeySequence("Ctrl+Shift+Z"));
  redo_action->setProperty("id", "redo");

  edit_menu->addSeparator();

  olive::MenuHelper.make_edit_functions_menu(edit_menu);

  edit_menu->addSeparator();

  edit_menu->addAction(tr("Select &All"), &olive::FocusFilter, SLOT(select_all()), QKeySequence("Ctrl+A"))->setProperty("id", "selectall");

  edit_menu->addAction(tr("Deselect All"), panel_timeline, SLOT(deselect()), QKeySequence("Ctrl+Shift+A"))->setProperty("id", "deselectall");

  edit_menu->addSeparator();

  olive::MenuHelper.make_clip_functions_menu(edit_menu);

  edit_menu->addSeparator();

  edit_menu->addAction(tr("Ripple to In Point"), panel_timeline, SLOT(ripple_to_in_point()), QKeySequence("Q"))->setProperty("id", "rippletoin");
  edit_menu->addAction(tr("Ripple to Out Point"), panel_timeline, SLOT(ripple_to_out_point()), QKeySequence("W"))->setProperty("id", "rippletoout");
  edit_menu->addAction(tr("Edit to In Point"), panel_timeline, SLOT(edit_to_in_point()), QKeySequence("Ctrl+Alt+Q"))->setProperty("id", "edittoin");
  edit_menu->addAction(tr("Edit to Out Point"), panel_timeline, SLOT(edit_to_out_point()), QKeySequence("Ctrl+Alt+W"))->setProperty("id", "edittoout");

  edit_menu->addSeparator();

  olive::MenuHelper.make_inout_menu(edit_menu);
  edit_menu->addAction(tr("Delete In/Out Point"), panel_timeline, SLOT(delete_inout()), QKeySequence(";"))->setProperty("id", "deleteinout");
  edit_menu->addAction(tr("Ripple Delete In/Out Point"), panel_timeline, SLOT(ripple_delete_inout()), QKeySequence("'"))->setProperty("id", "rippledeleteinout");

  edit_menu->addSeparator();

  edit_menu->addAction(tr("Set/Edit Marker"), &olive::FocusFilter, SLOT(set_marker()), QKeySequence("M"))->setProperty("id", "marker");

  // INITIALIZE VIEW MENU

  QMenu* view_menu = menuBar->addMenu(tr("&View"));
  connect(view_menu, SIGNAL(aboutToShow()), this, SLOT(viewMenu_About_To_Be_Shown()));

  view_menu->addAction(tr("Zoom In"), &olive::FocusFilter, SLOT(zoom_in()), QKeySequence("="))->setProperty("id", "zoomin");
  view_menu->addAction(tr("Zoom Out"), &olive::FocusFilter, SLOT(zoom_out()), QKeySequence("-"))->setProperty("id", "zoomout");
  view_menu->addAction(tr("Increase Track Height"), panel_timeline, SLOT(IncreaseTrackHeight()), QKeySequence("Ctrl+="))->setProperty("id", "vzoomin");
  view_menu->addAction(tr("Decrease Track Height"), panel_timeline, SLOT(DecreaseTrackHeight()), QKeySequence("Ctrl+-"))->setProperty("id", "vzoomout");

  show_all = view_menu->addAction(tr("Toggle Show All"), panel_timeline, SLOT(toggle_show_all()), QKeySequence("\\"));
  show_all->setProperty("id", "showall");
  show_all->setCheckable(true);

  view_menu->addSeparator();

  track_lines = view_menu->addAction(tr("Track Lines"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  track_lines->setProperty("id", "tracklines");
  track_lines->setCheckable(true);
  track_lines->setData(reinterpret_cast<quintptr>(&olive::CurrentConfig.show_track_lines));

  rectified_waveforms = view_menu->addAction(tr("Rectified Waveforms"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  rectified_waveforms->setProperty("id", "rectifiedwaveforms");
  rectified_waveforms->setCheckable(true);
  rectified_waveforms->setData(reinterpret_cast<quintptr>(&olive::CurrentConfig.rectified_waveforms));

  view_menu->addSeparator();

  frames_action = view_menu->addAction(tr("Frames"), &olive::MenuHelper, SLOT(set_timecode_view()));
  frames_action->setProperty("id", "modeframes");
  frames_action->setData(olive::kTimecodeFrames);
  frames_action->setCheckable(true);
  drop_frame_action = view_menu->addAction(tr("Drop Frame"), &olive::MenuHelper, SLOT(set_timecode_view()));
  drop_frame_action->setProperty("id", "modedropframe");
  drop_frame_action->setData(olive::kTimecodeDrop);
  drop_frame_action->setCheckable(true);
  nondrop_frame_action = view_menu->addAction(tr("Non-Drop Frame"), &olive::MenuHelper, SLOT(set_timecode_view()));
  nondrop_frame_action->setProperty("id", "modenondropframe");
  nondrop_frame_action->setData(olive::kTimecodeNonDrop);
  nondrop_frame_action->setCheckable(true);
  milliseconds_action = view_menu->addAction(tr("Milliseconds"), &olive::MenuHelper, SLOT(set_timecode_view()));
  milliseconds_action->setProperty("id", "milliseconds");
  milliseconds_action->setData(olive::kTimecodeMilliseconds);
  milliseconds_action->setCheckable(true);

  view_menu->addSeparator();

  QMenu* title_safe_area_menu = view_menu->addMenu(tr("Title/Action Safe Area"));

  title_safe_off = title_safe_area_menu->addAction(tr("Off"));
  title_safe_off->setProperty("id", "titlesafeoff");
  title_safe_off->setCheckable(true);
  title_safe_off->setData(qSNaN());
  connect(title_safe_off, SIGNAL(triggered(bool)), &olive::MenuHelper, SLOT(set_titlesafe_from_menu()));

  title_safe_default = title_safe_area_menu->addAction(tr("Default"));
  title_safe_default->setProperty("id", "titlesafedefault");
  title_safe_default->setCheckable(true);
  title_safe_default->setData(0.0);
  connect(title_safe_default, SIGNAL(triggered(bool)), &olive::MenuHelper, SLOT(set_titlesafe_from_menu()));

  title_safe_43 = title_safe_area_menu->addAction(tr("4:3"));
  title_safe_43->setProperty("id", "titlesafe43");
  title_safe_43->setCheckable(true);
  title_safe_43->setData(4.0/3.0);
  connect(title_safe_43, SIGNAL(triggered(bool)), &olive::MenuHelper, SLOT(set_titlesafe_from_menu()));

  title_safe_169 = title_safe_area_menu->addAction(tr("16:9"));
  title_safe_169->setProperty("id", "titlesafe169");
  title_safe_169->setCheckable(true);
  title_safe_169->setData(16.0/9.0);
  connect(title_safe_169, SIGNAL(triggered(bool)), &olive::MenuHelper, SLOT(set_titlesafe_from_menu()));

  title_safe_custom = title_safe_area_menu->addAction(tr("Custom"));
  title_safe_custom->setProperty("id", "titlesafecustom");
  title_safe_custom->setCheckable(true);
  title_safe_custom->setData(-1.0);
  connect(title_safe_custom, SIGNAL(triggered(bool)), &olive::MenuHelper, SLOT(set_titlesafe_from_menu()));

  view_menu->addSeparator();

  full_screen = view_menu->addAction(tr("Full Screen"), this, SLOT(toggle_full_screen()), QKeySequence("F11"));
  full_screen->setProperty("id", "fullscreen");
  full_screen->setCheckable(true);

  view_menu->addAction(tr("Full Screen Viewer"), &olive::FocusFilter, SLOT(set_viewer_fullscreen()))->setProperty("id", "fullscreenviewer");

  // INITIALIZE PLAYBACK MENU

  QMenu* playback_menu = menuBar->addMenu(tr("&Playback"));
  connect(playback_menu, SIGNAL(aboutToShow()), this, SLOT(playbackMenu_About_To_Be_Shown()));

  playback_menu->addAction(tr("Go to Start"), &olive::FocusFilter, SLOT(go_to_start()), QKeySequence("Home"))->setProperty("id", "gotostart");
  playback_menu->addAction(tr("Previous Frame"), &olive::FocusFilter, SLOT(prev_frame()), QKeySequence("Left"))->setProperty("id", "prevframe");
  playback_menu->addAction(tr("Play/Pause"), &olive::FocusFilter, SLOT(playpause()), QKeySequence("Space"))->setProperty("id", "playpause");
  playback_menu->addAction(tr("Play In to Out"), &olive::FocusFilter, SLOT(play_in_to_out()), QKeySequence("Shift+Space"))->setProperty("id", "playintoout");
  playback_menu->addAction(tr("Next Frame"), &olive::FocusFilter, SLOT(next_frame()), QKeySequence("Right"))->setProperty("id", "nextframe");
  playback_menu->addAction(tr("Go to End"), &olive::FocusFilter, SLOT(go_to_end()), QKeySequence("End"))->setProperty("id", "gotoend");
  playback_menu->addSeparator();
  playback_menu->addAction(tr("Go to Previous Cut"), panel_timeline, SLOT(previous_cut()), QKeySequence("Up"))->setProperty("id", "prevcut");
  playback_menu->addAction(tr("Go to Next Cut"), panel_timeline, SLOT(next_cut()), QKeySequence("Down"))->setProperty("id", "nextcut");
  playback_menu->addSeparator();
  playback_menu->addAction(tr("Go to In Point"), &olive::FocusFilter, SLOT(go_to_in()), QKeySequence("Shift+I"))->setProperty("id", "gotoin");
  playback_menu->addAction(tr("Go to Out Point"), &olive::FocusFilter, SLOT(go_to_out()), QKeySequence("Shift+O"))->setProperty("id", "gotoout");
  playback_menu->addSeparator();
  playback_menu->addAction(tr("Shuttle Left"), &olive::FocusFilter, SLOT(decrease_speed()), QKeySequence("J"))->setProperty("id", "decspeed");
  playback_menu->addAction(tr("Shuttle Stop"), &olive::FocusFilter, SLOT(pause()), QKeySequence("K"))->setProperty("id", "pause");
  playback_menu->addAction(tr("Shuttle Right"), &olive::FocusFilter, SLOT(increase_speed()), QKeySequence("L"))->setProperty("id", "incspeed");
  playback_menu->addSeparator();

  loop_action = playback_menu->addAction(tr("Loop"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  loop_action->setProperty("id", "loop");
  loop_action->setCheckable(true);
  loop_action->setData(reinterpret_cast<quintptr>(&olive::CurrentConfig.loop));

  // INITIALIZE WINDOW MENU

  window_menu = menuBar->addMenu(tr("&Window"));
  connect(window_menu, SIGNAL(aboutToShow()), this, SLOT(windowMenu_About_To_Be_Shown()));

  QAction* window_project_action = window_menu->addAction(tr("Project"), this, SLOT(toggle_panel_visibility()));
  window_project_action->setProperty("id", "panelproject");
  window_project_action->setCheckable(true);
  window_project_action->setData(reinterpret_cast<quintptr>(panel_project));

  QAction* window_effectcontrols_action = window_menu->addAction(tr("Effect Controls"), this, SLOT(toggle_panel_visibility()));
  window_effectcontrols_action->setProperty("id", "paneleffectcontrols");
  window_effectcontrols_action->setCheckable(true);
  window_effectcontrols_action->setData(reinterpret_cast<quintptr>(panel_effect_controls));

  QAction* window_timeline_action = window_menu->addAction(tr("Timeline"), this, SLOT(toggle_panel_visibility()));
  window_timeline_action->setProperty("id", "paneltimeline");
  window_timeline_action->setCheckable(true);
  window_timeline_action->setData(reinterpret_cast<quintptr>(panel_timeline));

  QAction* window_graph_editor_action = window_menu->addAction(tr("Graph Editor"), this, SLOT(toggle_panel_visibility()));
  window_graph_editor_action->setProperty("id", "panelgrapheditor");
  window_graph_editor_action->setCheckable(true);
  window_graph_editor_action->setData(reinterpret_cast<quintptr>(panel_graph_editor));

  QAction* window_footageviewer_action = window_menu->addAction(tr("Media Viewer"), this, SLOT(toggle_panel_visibility()));
  window_footageviewer_action->setProperty("id", "panelfootageviewer");
  window_footageviewer_action->setCheckable(true);
  window_footageviewer_action->setData(reinterpret_cast<quintptr>(panel_footage_viewer));

  QAction* window_sequenceviewer_action = window_menu->addAction(tr("Sequence Viewer"), this, SLOT(toggle_panel_visibility()));
  window_sequenceviewer_action->setProperty("id", "panelsequenceviewer");
  window_sequenceviewer_action->setCheckable(true);
  window_sequenceviewer_action->setData(reinterpret_cast<quintptr>(panel_sequence_viewer));

  window_menu->addSeparator();

  window_menu->addAction(tr("Maximize Panel"), this, SLOT(maximize_panel()), QKeySequence("`"))->setProperty("id", "maximizepanel");

  window_menu->addSeparator();

  window_menu->addAction(tr("Reset to Default Layout"), this, SLOT(reset_layout()))->setProperty("id", "resetdefaultlayout");

  // INITIALIZE TOOLS MENU

  QMenu* tools_menu = menuBar->addMenu(tr("&Tools"));
  connect(tools_menu, SIGNAL(aboutToShow()), this, SLOT(toolMenu_About_To_Be_Shown()));

  pointer_tool_action = tools_menu->addAction(tr("Pointer Tool"), &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("V"));
  pointer_tool_action->setProperty("id", "pointertool");
  pointer_tool_action->setCheckable(true);
  pointer_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline->toolArrowButton));

  edit_tool_action = tools_menu->addAction(tr("Edit Tool"), &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("X"));
  edit_tool_action->setProperty("id", "edittool");
  edit_tool_action->setCheckable(true);
  edit_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline->toolEditButton));

  ripple_tool_action = tools_menu->addAction(tr("Ripple Tool"), &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("B"));
  ripple_tool_action->setProperty("id", "rippletool");
  ripple_tool_action->setCheckable(true);
  ripple_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline->toolRippleButton));

  razor_tool_action = tools_menu->addAction(tr("Razor Tool"), &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("C"));
  razor_tool_action->setProperty("id", "razortool");
  razor_tool_action->setCheckable(true);
  razor_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline->toolRazorButton));

  slip_tool_action = tools_menu->addAction(tr("Slip Tool"), &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("Y"));
  slip_tool_action->setProperty("id", "sliptool");
  slip_tool_action->setCheckable(true);
  slip_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline->toolSlipButton));

  slide_tool_action = tools_menu->addAction(tr("Slide Tool"), &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("U"));
  slide_tool_action->setProperty("id", "slidetool");
  slide_tool_action->setCheckable(true);
  slide_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline->toolSlideButton));

  hand_tool_action = tools_menu->addAction(tr("Hand Tool"), &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("H"));
  hand_tool_action->setProperty("id", "handtool");
  hand_tool_action->setCheckable(true);
  hand_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline->toolHandButton));

  transition_tool_action = tools_menu->addAction(tr("Transition Tool"), &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("T"));
  transition_tool_action->setProperty("id", "transitiontool");
  transition_tool_action->setCheckable(true);
  transition_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline->toolTransitionButton));

  tools_menu->addSeparator();

  snap_toggle = tools_menu->addAction(tr("Enable Snapping"), &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("S"));
  snap_toggle->setProperty("id", "snapping");
  snap_toggle->setCheckable(true);
  snap_toggle->setData(reinterpret_cast<quintptr>(panel_timeline->snappingButton));

  tools_menu->addSeparator();

  selecting_also_seeks = tools_menu->addAction(tr("Selecting Also Seeks"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  selecting_also_seeks->setProperty("id", "selectingalsoseeks");
  selecting_also_seeks->setCheckable(true);
  selecting_also_seeks->setData(reinterpret_cast<quintptr>(&olive::CurrentConfig.select_also_seeks));

  edit_tool_also_seeks = tools_menu->addAction(tr("Edit Tool Also Seeks"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  edit_tool_also_seeks->setProperty("id", "editalsoseeks");
  edit_tool_also_seeks->setCheckable(true);
  edit_tool_also_seeks->setData(reinterpret_cast<quintptr>(&olive::CurrentConfig.edit_tool_also_seeks));

  edit_tool_selects_links = tools_menu->addAction(tr("Edit Tool Selects Links"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  edit_tool_selects_links->setProperty("id", "editselectslinks");
  edit_tool_selects_links->setCheckable(true);
  edit_tool_selects_links->setData(reinterpret_cast<quintptr>(&olive::CurrentConfig.edit_tool_selects_links));

  seek_also_selects = tools_menu->addAction(tr("Seek Also Selects"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  seek_also_selects->setProperty("id", "seekalsoselects");
  seek_also_selects->setCheckable(true);
  seek_also_selects->setData(reinterpret_cast<quintptr>(&olive::CurrentConfig.seek_also_selects));

  seek_to_end_of_pastes = tools_menu->addAction(tr("Seek to the End of Pastes"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  seek_to_end_of_pastes->setProperty("id", "seektoendofpastes");
  seek_to_end_of_pastes->setCheckable(true);
  seek_to_end_of_pastes->setData(reinterpret_cast<quintptr>(&olive::CurrentConfig.paste_seeks));

  scroll_wheel_zooms = tools_menu->addAction(tr("Scroll Wheel Zooms"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  scroll_wheel_zooms->setProperty("id", "scrollwheelzooms");
  scroll_wheel_zooms->setCheckable(true);
  scroll_wheel_zooms->setData(reinterpret_cast<quintptr>(&olive::CurrentConfig.scroll_zooms));

  enable_drag_files_to_timeline = tools_menu->addAction(tr("Enable Drag Files to Timeline"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  enable_drag_files_to_timeline->setProperty("id", "enabledragfilestotimeline");
  enable_drag_files_to_timeline->setCheckable(true);
  enable_drag_files_to_timeline->setData(reinterpret_cast<quintptr>(&olive::CurrentConfig.enable_drag_files_to_timeline));

  autoscale_by_default = tools_menu->addAction(tr("Auto-Scale By Default"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  autoscale_by_default->setProperty("id", "autoscalebydefault");
  autoscale_by_default->setCheckable(true);
  autoscale_by_default->setData(reinterpret_cast<quintptr>(&olive::CurrentConfig.autoscale_by_default));

  enable_seek_to_import = tools_menu->addAction(tr("Enable Seek to Import"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  enable_seek_to_import->setProperty("id", "enableseektoimport");
  enable_seek_to_import->setCheckable(true);
  enable_seek_to_import->setData(reinterpret_cast<quintptr>(&olive::CurrentConfig.enable_seek_to_import));

  enable_audio_scrubbing = tools_menu->addAction(tr("Audio Scrubbing"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  enable_audio_scrubbing->setProperty("id", "audioscrubbing");
  enable_audio_scrubbing->setCheckable(true);
  enable_audio_scrubbing->setData(reinterpret_cast<quintptr>(&olive::CurrentConfig.enable_audio_scrubbing));

  enable_drop_on_media_to_replace = tools_menu->addAction(tr("Enable Drop on Media to Replace"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  enable_drop_on_media_to_replace->setProperty("id", "enabledropmediareplace");
  enable_drop_on_media_to_replace->setCheckable(true);
  enable_drop_on_media_to_replace->setData(reinterpret_cast<quintptr>(&olive::CurrentConfig.drop_on_media_to_replace));

  enable_hover_focus = tools_menu->addAction(tr("Enable Hover Focus"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  enable_hover_focus->setProperty("id", "hoverfocus");
  enable_hover_focus->setCheckable(true);
  enable_hover_focus->setData(reinterpret_cast<quintptr>(&olive::CurrentConfig.hover_focus));

  set_name_and_marker = tools_menu->addAction(tr("Ask For Name When Setting Marker"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  set_name_and_marker->setProperty("id", "asknamemarkerset");
  set_name_and_marker->setCheckable(true);
  set_name_and_marker->setData(reinterpret_cast<quintptr>(&olive::CurrentConfig.set_name_with_marker));

  tools_menu->addSeparator();

  no_autoscroll = tools_menu->addAction(tr("No Auto-Scroll"), &olive::MenuHelper, SLOT(set_autoscroll()));
  no_autoscroll->setProperty("id", "autoscrollno");
  no_autoscroll->setData(olive::AUTOSCROLL_NO_SCROLL);
  no_autoscroll->setCheckable(true);

  page_autoscroll = tools_menu->addAction(tr("Page Auto-Scroll"), &olive::MenuHelper, SLOT(set_autoscroll()));
  page_autoscroll->setProperty("id", "autoscrollpage");
  page_autoscroll->setData(olive::AUTOSCROLL_PAGE_SCROLL);
  page_autoscroll->setCheckable(true);

  smooth_autoscroll = tools_menu->addAction(tr("Smooth Auto-Scroll"), &olive::MenuHelper, SLOT(set_autoscroll()));
  smooth_autoscroll->setProperty("id", "autoscrollsmooth");
  smooth_autoscroll->setData(olive::AUTOSCROLL_SMOOTH_SCROLL);
  smooth_autoscroll->setCheckable(true);

  tools_menu->addSeparator();

  tools_menu->addAction(tr("Preferences"), olive::Global.get(), SLOT(open_preferences()), QKeySequence("Ctrl+,"))->setProperty("id", "prefs");

#ifdef QT_DEBUG
  tools_menu->addAction(tr("Clear Undo"), olive::Global.get(), SLOT(clear_undo_stack()))->setProperty("id", "clearundo");
#endif

  // INITIALIZE HELP MENU

  QMenu* help_menu = menuBar->addMenu(tr("&Help"));

  help_menu->addAction(tr("A&ction Search"), olive::Global.get(), SLOT(open_action_search()), QKeySequence("/"))->setProperty("id", "actionsearch");

  help_menu->addSeparator();

  help_menu->addAction(tr("Debug Log"), olive::Global.get(), SLOT(open_debug_log()))->setProperty("id", "debuglog");

  help_menu->addSeparator();

  help_menu->addAction(tr("&About..."), olive::Global.get(), SLOT(open_about_dialog()))->setProperty("id", "about");

  load_shortcuts(get_config_path() + "/shortcuts");
}

void MainWindow::updateTitle() {
  setWindowTitle(QString("%1 - %2[*]").arg(olive::AppName,
                                           (olive::ActiveProjectFilename.isEmpty()) ?
                                             tr("<untitled>") : olive::ActiveProjectFilename)
                 );
}

void MainWindow::closeEvent(QCloseEvent *e) {
  if (olive::Global->can_close_project()) {
    // stop proxy generator thread
    proxy_generator.cancel();

    panel_effect_controls->clear_effects(true);

    set_sequence(nullptr);

    panel_footage_viewer->viewer_widget->close_window();
    panel_sequence_viewer->viewer_widget->close_window();

    panel_footage_viewer->set_main_sequence();

    QString data_dir = get_data_path();
    QString config_path = get_config_path();
    if (!data_dir.isEmpty() && !autorecovery_filename.isEmpty()) {
      if (QFile::exists(autorecovery_filename)) {
        QFile::rename(autorecovery_filename, autorecovery_filename + "." + QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmss"));
      }
    }
    if (!config_path.isEmpty()) {
      QDir config_dir = QDir(config_path);

      QString config_fn = config_dir.filePath("config.xml");

      // save settings
      olive::CurrentConfig.save(config_fn);

      // save panel layout
      QFile panel_config(config_path + "/layout");
      if (panel_config.open(QFile::WriteOnly)) {
        panel_config.write(saveState(0));
        panel_config.close();
      } else {
        qCritical() << "Failed to save layout";
      }

      save_shortcuts(config_path + "/shortcuts");
    }

    stop_audio();

    e->accept();
  } else {
    e->ignore();
  }
}

void MainWindow::paintEvent(QPaintEvent *event) {
  QMainWindow::paintEvent(event);

  if (first_show) {
    first_show = false;
    emit finished_first_paint();
  }
}

void MainWindow::reset_layout() {
  setup_layout(true);
}



void MainWindow::maximize_panel() {
  // toggles between normal state and a state of one panel being maximized
  if (temp_panel_state.isEmpty()) {
    // get currently hovered panel
    QDockWidget* focused_panel = get_focused_panel(true);

    // if the mouse is in fact hovering over a panel
    if (focused_panel != nullptr) {
      // store the current state of panels
      temp_panel_state = saveState();

      // remove all dock widgets (kind of painful having to do each individually)
      if (focused_panel != panel_project) removeDockWidget(panel_project);
      if (focused_panel != panel_effect_controls) removeDockWidget(panel_effect_controls);
      if (focused_panel != panel_timeline) removeDockWidget(panel_timeline);
      if (focused_panel != panel_sequence_viewer) removeDockWidget(panel_sequence_viewer);
      if (focused_panel != panel_footage_viewer) removeDockWidget(panel_footage_viewer);
      if (focused_panel != panel_graph_editor) removeDockWidget(panel_graph_editor);
    }
  } else {
    // we must be maximized, restore previous state
    restoreState(temp_panel_state);

    // clear temp panel state for next maximize call
    temp_panel_state.clear();
  }
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

void MainWindow::playbackMenu_About_To_Be_Shown() {
  olive::MenuHelper.set_bool_action_checked(loop_action);
}

void MainWindow::viewMenu_About_To_Be_Shown() {
  olive::MenuHelper.set_bool_action_checked(track_lines);

  olive::MenuHelper.set_int_action_checked(frames_action, olive::CurrentConfig.timecode_view);
  olive::MenuHelper.set_int_action_checked(drop_frame_action, olive::CurrentConfig.timecode_view);
  olive::MenuHelper.set_int_action_checked(nondrop_frame_action, olive::CurrentConfig.timecode_view);
  olive::MenuHelper.set_int_action_checked(milliseconds_action, olive::CurrentConfig.timecode_view);

  title_safe_off->setChecked(!olive::CurrentConfig.show_title_safe_area);
  title_safe_default->setChecked(olive::CurrentConfig.show_title_safe_area
                                 && !olive::CurrentConfig.use_custom_title_safe_ratio);
  title_safe_43->setChecked(olive::CurrentConfig.show_title_safe_area
                            && olive::CurrentConfig.use_custom_title_safe_ratio
                            && qFuzzyCompare(olive::CurrentConfig.custom_title_safe_ratio, title_safe_43->data().toDouble()));
  title_safe_169->setChecked(olive::CurrentConfig.show_title_safe_area
                             && olive::CurrentConfig.use_custom_title_safe_ratio
                             && qFuzzyCompare(olive::CurrentConfig.custom_title_safe_ratio, title_safe_169->data().toDouble()));
  title_safe_custom->setChecked(olive::CurrentConfig.show_title_safe_area
                                && olive::CurrentConfig.use_custom_title_safe_ratio
                                && !title_safe_43->isChecked()
                                && !title_safe_169->isChecked());

  full_screen->setChecked(windowState() == Qt::WindowFullScreen);

  show_all->setChecked(panel_timeline->showing_all);
}

void MainWindow::toolMenu_About_To_Be_Shown() {
  olive::MenuHelper.set_button_action_checked(pointer_tool_action);
  olive::MenuHelper.set_button_action_checked(edit_tool_action);
  olive::MenuHelper.set_button_action_checked(ripple_tool_action);
  olive::MenuHelper.set_button_action_checked(razor_tool_action);
  olive::MenuHelper.set_button_action_checked(slip_tool_action);
  olive::MenuHelper.set_button_action_checked(slide_tool_action);
  olive::MenuHelper.set_button_action_checked(hand_tool_action);
  olive::MenuHelper.set_button_action_checked(transition_tool_action);
  olive::MenuHelper.set_button_action_checked(snap_toggle);

  olive::MenuHelper.set_bool_action_checked(selecting_also_seeks);
  olive::MenuHelper.set_bool_action_checked(edit_tool_also_seeks);
  olive::MenuHelper.set_bool_action_checked(edit_tool_selects_links);
  olive::MenuHelper.set_bool_action_checked(seek_to_end_of_pastes);
  olive::MenuHelper.set_bool_action_checked(scroll_wheel_zooms);
  olive::MenuHelper.set_bool_action_checked(rectified_waveforms);
  olive::MenuHelper.set_bool_action_checked(enable_drag_files_to_timeline);
  olive::MenuHelper.set_bool_action_checked(autoscale_by_default);
  olive::MenuHelper.set_bool_action_checked(enable_seek_to_import);
  olive::MenuHelper.set_bool_action_checked(enable_audio_scrubbing);
  olive::MenuHelper.set_bool_action_checked(enable_drop_on_media_to_replace);
  olive::MenuHelper.set_bool_action_checked(enable_hover_focus);
  olive::MenuHelper.set_bool_action_checked(set_name_and_marker);
  olive::MenuHelper.set_bool_action_checked(seek_also_selects);

  olive::MenuHelper.set_int_action_checked(no_autoscroll, olive::CurrentConfig.autoscroll);
  olive::MenuHelper.set_int_action_checked(page_autoscroll, olive::CurrentConfig.autoscroll);
  olive::MenuHelper.set_int_action_checked(smooth_autoscroll, olive::CurrentConfig.autoscroll);
}

void MainWindow::toggle_panel_visibility() {
  QAction* action = static_cast<QAction*>(sender());
  QDockWidget* w = reinterpret_cast<QDockWidget*>(action->data().value<quintptr>());
  w->setVisible(!w->isVisible());

  // layout has changed, we're no longer in maximized panel mode,
  // so we clear this byte array
  temp_panel_state.clear();
}

void MainWindow::fileMenu_About_To_Be_Shown() {
  if (recent_projects.size() > 0) {
    open_recent->clear();
    open_recent->setEnabled(true);
    for (int i=0;i<recent_projects.size();i++) {
      QAction* action = open_recent->addAction(recent_projects.at(i));
      action->setProperty("keyignore", true);
      action->setData(i);
      connect(action, SIGNAL(triggered()), &olive::MenuHelper, SLOT(open_recent_from_menu()));
    }
    open_recent->addSeparator();

    open_recent->addAction(clear_open_recent_action);
  } else {
    open_recent->setEnabled(false);
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
