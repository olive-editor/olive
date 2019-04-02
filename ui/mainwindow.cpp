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

#include "global/global.h"
#include "ui/menuhelper.h"
#include "project/projectelements.h"
#include "timeline/clip.h"
#include "global/config.h"
#include "global/path.h"
#include "global/debug.h"
#include "project/proxygenerator.h"
#include "project/projectfilter.h"
#include "ui/sourcetable.h"
#include "ui/viewerwidget.h"
#include "ui/sourceiconview.h"
#include "ui/timelineheader.h"
#include "ui/icons.h"
#include "ui/cursors.h"
#include "ui/focusfilter.h"
#include "panels/panels.h"
#include "dialogs/debugdialog.h"
#include "rendering/audio.h"
#include "rendering/renderfunctions.h"
#include "undo/undostack.h"
#include "effects/effectloaders.h"

MainWindow* olive::MainWindow;

void MainWindow::setup_layout(bool reset) {
  // load panels from file
  if (!reset) {
    QFile panel_config(get_config_dir().filePath("layout"));
    if (panel_config.exists() && panel_config.open(QFile::ReadOnly)) {

      // default to resetting unless we find layout data in the XML file
      reset = true;

      // read XML layout file
      QXmlStreamReader stream(&panel_config);

      // loop through XML for all data
      while (!stream.atEnd()) {
        stream.readNext();

        if (stream.name() == "panels" && stream.isStartElement()) {

          // element contains MainWindow layout data to restore
          stream.readNext();
          restoreState(QByteArray::fromBase64(stream.text().toUtf8()), 0);
          reset = false;

        } else if (stream.name() == "panel" && stream.isStartElement()) {

          // element contains layout data specific to a panel, we'll find the panel and load it

          // get panel name from XML attribute
          QString panel_name;
          const QXmlStreamAttributes& attributes = stream.attributes();
          for (int i=0;i<attributes.size();i++) {
            const QXmlStreamAttribute& attr = attributes.at(i);
            if (attr.name() == "name") {
              panel_name = attr.value().toString();
              break;
            }
          }

          if (panel_name.isEmpty()) {
            qWarning() << "Layout file specified data for a panel but didn't specify a name. Layout wasn't loaded.";
          } else {
            // loop through panels for a panel with the same name

            bool found_panel = false;

            for (int i=0;i<olive::panels.size();i++) {

              Panel* panel = olive::panels.at(i);
              if (panel->objectName() == panel_name) {

                // found the panel, so we can load its state
                stream.readNext();
                panel->LoadLayoutState(QByteArray::fromBase64(stream.text().toUtf8()));

                // we found it, no more need to loop through panels
                found_panel = true;

                break;

              }

            }

            if (!found_panel) {
              qWarning() << "Panel specified in layout data doesn't exist. Layout wasn't loaded.";
            }

          }

        }

      }

      panel_config.close();
    } else {
      reset = true;
    }
  }

  if (reset) {
    // remove all panels from the main window
    for (int i=0;i<olive::panels.size();i++) {
      removeDockWidget(olive::panels.at(i));
    }

    addDockWidget(Qt::TopDockWidgetArea, panel_project.first());
    addDockWidget(Qt::TopDockWidgetArea, panel_graph_editor);
    addDockWidget(Qt::TopDockWidgetArea, panel_footage_viewer);
    tabifyDockWidget(panel_footage_viewer, panel_effect_controls);
    panel_footage_viewer->raise();
    addDockWidget(Qt::TopDockWidgetArea, panel_sequence_viewer);
    addDockWidget(Qt::BottomDockWidgetArea, panel_timeline.first());

    panel_project.first()->show();
    panel_effect_controls->show();
    panel_footage_viewer->show();
    panel_sequence_viewer->show();
    panel_timeline.first()->show();
    panel_graph_editor->hide();

    panel_project.first()->setFloating(false);
    panel_effect_controls->setFloating(false);
    panel_footage_viewer->setFloating(false);
    panel_sequence_viewer->setFloating(false);
    panel_timeline.first()->setFloating(false);
    panel_graph_editor->setFloating(true);

    resizeDocks({panel_project.first(), panel_footage_viewer, panel_sequence_viewer},
                {width()/3, width()/3, width()/3},
                Qt::Horizontal);

    resizeDocks({panel_project.first(), panel_timeline.first()},
                {height()/2, height()/2},
                Qt::Vertical);
  }

  layout()->update();
}

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  first_show(true)
{
  EffectInit::StartLoading();

  olive::cursor::Initialize();

  open_debug_file();

  olive::DebugDialog = new DebugDialog(this);

  olive::MainWindow = this;

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
      olive::Global->load_recent_projects();
    }
  }
  QString config_path = get_config_path();
  if (!config_path.isEmpty()) {
    QDir config_dir(config_path);
    config_dir.mkpath(".");
    QString config_fn = config_dir.filePath("config.xml");
    if (QFileInfo::exists(config_fn)) {
      olive::config.load(config_fn);
    }
  }

  Restyle();

  olive::icon::Initialize();

  // Load OpenColorIO configuration if set
  if (olive::config.enable_color_management && !olive::config.ocio_config_path.isEmpty()) {
    try {
      OCIO::SetCurrentConfig(OCIO::Config::CreateFromFile(olive::config.ocio_config_path.toUtf8()));
    } catch (OCIO::Exception& e) {
      QMessageBox::critical(this,
                            tr("OpenColorIO Config Error"),
                            tr("Failed to set OpenColorIO configuration: %1").arg(e.what()),
                            QMessageBox::Ok);
    }
  }

  alloc_panels(this);

  // populate menu bars
  setup_menus();

  QStatusBar* statusBar = new QStatusBar(this);
  statusBar->showMessage(tr("Welcome to %1").arg(olive::AppName));
  setStatusBar(statusBar);

  olive::Global->check_for_autorecovery_file();

  // set up output audio device
  init_audio();

  // start omnipotent proxy generator process
  olive::proxy_generator.start();

  // load preferred language from file
  olive::Global->load_translation_from_config();

  // set default strings
  Retranslate();
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

bool MainWindow::load_css_from_file(const QString &fn) {
  QFile css_file(fn);
  if (css_file.exists() && css_file.open(QFile::ReadOnly)) {
    setStyleSheet(css_file.readAll());
    css_file.close();
    return true;
  }
  return false;
}

void MainWindow::Restyle()
{
  // Set up UI style
  if (!olive::styling::UseNativeUI()) {
    qApp->setStyle(QStyleFactory::create("Fusion"));

    // Set up whether to load custom CSS or default CSS+palette
    if (!olive::config.css_path.isEmpty()
        && load_css_from_file(olive::config.css_path)) {

      qApp->setPalette(qApp->style()->standardPalette());

    } else {

      // set default palette
      QPalette palette;

      if (olive::config.style == olive::styling::kOliveDefaultLight) {

        palette.setColor(QPalette::Window, QColor(208, 208, 208));
        palette.setColor(QPalette::WindowText, Qt::black);
        palette.setColor(QPalette::Base, QColor(240, 240, 240));
        palette.setColor(QPalette::AlternateBase, QColor(208, 208, 208));
        palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
        palette.setColor(QPalette::ToolTipText, Qt::black);
        palette.setColor(QPalette::Text, Qt::black);
        palette.setColor(QPalette::Button, QColor(208, 208, 208));
        palette.setColor(QPalette::ButtonText, Qt::black);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(208, 208, 208));
        palette.setColor(QPalette::Link, QColor(42, 130, 218));
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::HighlightedText, Qt::white);

        /* Olive Mid
        palette.setColor(QPalette::Window, QColor(128, 128, 128));
        palette.setColor(QPalette::WindowText, Qt::black);
        palette.setColor(QPalette::Base, QColor(192, 192, 192));
        palette.setColor(QPalette::AlternateBase, QColor(128, 128, 128));
        palette.setColor(QPalette::ToolTipBase, QColor(192, 192, 192));
        palette.setColor(QPalette::ToolTipText, Qt::black);
        palette.setColor(QPalette::Text, Qt::black);
        palette.setColor(QPalette::Button, QColor(128, 128, 128));
        palette.setColor(QPalette::ButtonText, Qt::black);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));
        palette.setColor(QPalette::Link, QColor(42, 130, 218));
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::HighlightedText, Qt::black);
        */

      } else {

        palette.setColor(QPalette::Window, QColor(53,53,53));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(25,25,25));
        palette.setColor(QPalette::AlternateBase, QColor(53,53,53));
        palette.setColor(QPalette::ToolTipBase, QColor(25,25,25));
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(53,53,53));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));
        palette.setColor(QPalette::Link, QColor(42, 130, 218));
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::HighlightedText, Qt::white);

        // set default CSS
        QString stylesheet = "QPushButton::checked { background: rgb(25, 25, 25); }";

        // Windows menus have the option of being native, so we may not need this CSS
#ifdef Q_OS_WIN
        if (!olive::CurrentConfig.use_native_menu_styling) {
#endif
          stylesheet.append("QMenu::separator { background: #404040; }");
#ifdef Q_OS_WIN
        }
#endif
        setStyleSheet(stylesheet);

      }

      qApp->setPalette(palette);

    }
  }
}

void MainWindow::editMenu_About_To_Be_Shown() {
  undo_action->setEnabled(olive::undo_stack.canUndo());
  redo_action->setEnabled(olive::undo_stack.canRedo());
}

void MainWindow::setup_menus() {
  QMenuBar* menuBar = new QMenuBar(this);

  if (olive::config.use_native_menu_styling) {
    OliveGlobal::SetNativeStyling(menuBar);
  }

  setMenuBar(menuBar);

  olive::MenuHelper.InitializeSharedMenus();

  // INITIALIZE FILE MENU

  file_menu = MenuHelper::create_submenu(menuBar, this, SLOT(fileMenu_About_To_Be_Shown()));

  new_menu = MenuHelper::create_submenu(file_menu);
  olive::MenuHelper.make_new_menu(new_menu);

  open_project = MenuHelper::create_menu_action(file_menu, "openproj", olive::Global.get(), SLOT(OpenProject()), QKeySequence("Ctrl+O"));

  open_recent = MenuHelper::create_submenu(file_menu);

  clear_open_recent_action = MenuHelper::create_menu_action(nullptr, "clearopenrecent", olive::Global.get(), SLOT(clear_recent_projects()));

  save_project = MenuHelper::create_menu_action(file_menu, "saveproj", olive::Global.get(), SLOT(save_project()), QKeySequence("Ctrl+S"));

  save_project_as = MenuHelper::create_menu_action(file_menu, "saveprojas", olive::Global.get(), SLOT(save_project_as()), QKeySequence("Ctrl+Shift+S"));

  file_menu->addSeparator();

  import_action = MenuHelper::create_menu_action(file_menu, "import", olive::Global.get(), SLOT(open_import_dialog()), QKeySequence("Ctrl+I"));

  file_menu->addSeparator();

  export_action = MenuHelper::create_menu_action(file_menu, "export", olive::Global.get(), SLOT(open_export_dialog()), QKeySequence("Ctrl+M"));

  file_menu->addSeparator();

  exit_action = MenuHelper::create_menu_action(file_menu, "exit", this, SLOT(close()));

  // INITIALIZE EDIT MENU

  edit_menu = MenuHelper::create_submenu(menuBar, this, SLOT(editMenu_About_To_Be_Shown()));

  undo_action = MenuHelper::create_menu_action(edit_menu, "undo", olive::Global.get(), SLOT(undo()), QKeySequence("Ctrl+Z"));
  redo_action = MenuHelper::create_menu_action(edit_menu, "redo", olive::Global.get(), SLOT(redo()), QKeySequence("Ctrl+Shift+Z"));

  edit_menu->addSeparator();

  olive::MenuHelper.make_edit_functions_menu(edit_menu);

  edit_menu->addSeparator();

  select_all_action = MenuHelper::create_menu_action(edit_menu, "selectall", &olive::FocusFilter, SLOT(select_all()), QKeySequence("Ctrl+A"));
  deselect_all_action = MenuHelper::create_menu_action(edit_menu, "deselectall", panel_timeline.first(), SLOT(deselect()), QKeySequence("Ctrl+Shift+A"));

  edit_menu->addSeparator();

  olive::MenuHelper.make_clip_functions_menu(edit_menu);

  edit_menu->addSeparator();

  ripple_to_in_point_ = MenuHelper::create_menu_action(edit_menu, "rippletoin", panel_timeline.first(), SLOT(ripple_to_in_point()), QKeySequence("Q"));
  ripple_to_out_point_ = MenuHelper::create_menu_action(edit_menu, "rippletoout", panel_timeline.first(), SLOT(ripple_to_out_point()), QKeySequence("W"));
  edit_to_in_point_ = MenuHelper::create_menu_action(edit_menu, "edittoin", panel_timeline.first(), SLOT(edit_to_in_point()), QKeySequence("Ctrl+Alt+Q"));
  edit_to_out_point_ = MenuHelper::create_menu_action(edit_menu, "edittoout", panel_timeline.first(), SLOT(edit_to_out_point()), QKeySequence("Ctrl+Alt+W"));

  edit_menu->addSeparator();

  olive::MenuHelper.make_inout_menu(edit_menu);
  delete_inout_point_ = MenuHelper::create_menu_action(edit_menu, "deleteinout", panel_timeline.first(), SLOT(delete_inout()), QKeySequence(";"));
  ripple_delete_inout_point_ = MenuHelper::create_menu_action(edit_menu, "rippledeleteinout", panel_timeline.first(), SLOT(ripple_delete_inout()), QKeySequence("'"));

  edit_menu->addSeparator();

  setedit_marker_ = MenuHelper::create_menu_action(edit_menu, "marker", &olive::FocusFilter, SLOT(set_marker()), QKeySequence("M"));

  // INITIALIZE VIEW MENU

  view_menu = MenuHelper::create_submenu(menuBar, this, SLOT(viewMenu_About_To_Be_Shown()));

  zoom_in_ = MenuHelper::create_menu_action(view_menu, "zoomin", &olive::FocusFilter, SLOT(zoom_in()), QKeySequence("="));
  zoom_out_ = MenuHelper::create_menu_action(view_menu, "zoomout", &olive::FocusFilter, SLOT(zoom_out()), QKeySequence("-"));
  increase_track_height_ = MenuHelper::create_menu_action(view_menu, "vzoomin", panel_timeline.first(), SLOT(IncreaseTrackHeight()), QKeySequence("Ctrl+="));
  decrease_track_height_ = MenuHelper::create_menu_action(view_menu, "vzoomout", panel_timeline.first(), SLOT(DecreaseTrackHeight()), QKeySequence("Ctrl+-"));

  show_all = MenuHelper::create_menu_action(view_menu, "showall", panel_timeline.first(), SLOT(toggle_show_all()), QKeySequence("\\"));
  show_all->setCheckable(true);

  view_menu->addSeparator();

  rectified_waveforms = MenuHelper::create_menu_action(view_menu, "rectifiedwaveforms", &olive::MenuHelper, SLOT(toggle_bool_action()));
  rectified_waveforms->setCheckable(true);
  rectified_waveforms->setData(reinterpret_cast<quintptr>(&olive::config.rectified_waveforms));

  view_menu->addSeparator();

  QActionGroup* frame_view_mode_group = new QActionGroup(this);

  frames_action = MenuHelper::create_menu_action(view_menu, "modeframes", &olive::MenuHelper, SLOT(set_timecode_view()));
  frames_action->setData(olive::kTimecodeFrames);
  frames_action->setCheckable(true);
  frame_view_mode_group->addAction(frames_action);

  drop_frame_action = MenuHelper::create_menu_action(view_menu, "modedropframe", &olive::MenuHelper, SLOT(set_timecode_view()));
  drop_frame_action->setData(olive::kTimecodeDrop);
  drop_frame_action->setCheckable(true);
  frame_view_mode_group->addAction(drop_frame_action);

  nondrop_frame_action = MenuHelper::create_menu_action(view_menu, "modenondropframe", &olive::MenuHelper, SLOT(set_timecode_view()));
  nondrop_frame_action->setData(olive::kTimecodeNonDrop);
  nondrop_frame_action->setCheckable(true);
  frame_view_mode_group->addAction(nondrop_frame_action);

  milliseconds_action = MenuHelper::create_menu_action(view_menu, "milliseconds", &olive::MenuHelper, SLOT(set_timecode_view()));
  milliseconds_action->setData(olive::kTimecodeMilliseconds);
  milliseconds_action->setCheckable(true);
  frame_view_mode_group->addAction(milliseconds_action);

  view_menu->addSeparator();

  title_safe_area_menu = MenuHelper::create_submenu(view_menu);

  QActionGroup* title_safe_group = new QActionGroup(this);

  title_safe_off = MenuHelper::create_menu_action(title_safe_area_menu, "titlesafeoff", &olive::MenuHelper, SLOT(set_titlesafe_from_menu()));
  title_safe_off->setCheckable(true);
  title_safe_off->setData(qSNaN());
  title_safe_group->addAction(title_safe_off);

  title_safe_default = MenuHelper::create_menu_action(title_safe_area_menu, "titlesafedefault", &olive::MenuHelper, SLOT(set_titlesafe_from_menu()));
  title_safe_default->setCheckable(true);
  title_safe_default->setData(0.0);
  title_safe_group->addAction(title_safe_default);

  title_safe_43 = MenuHelper::create_menu_action(title_safe_area_menu, "titlesafe43", &olive::MenuHelper, SLOT(set_titlesafe_from_menu()));
  title_safe_43->setCheckable(true);
  title_safe_43->setData(4.0/3.0);
  title_safe_group->addAction(title_safe_43);

  title_safe_169 = MenuHelper::create_menu_action(title_safe_area_menu, "titlesafe169", &olive::MenuHelper, SLOT(set_titlesafe_from_menu()));
  title_safe_169->setCheckable(true);
  title_safe_169->setData(16.0/9.0);
  title_safe_group->addAction(title_safe_169);

  title_safe_custom = MenuHelper::create_menu_action(title_safe_area_menu, "titlesafecustom", &olive::MenuHelper, SLOT(set_titlesafe_from_menu()));
  title_safe_custom->setCheckable(true);
  title_safe_custom->setData(-1.0);
  title_safe_group->addAction(title_safe_custom);

  view_menu->addSeparator();

  full_screen = MenuHelper::create_menu_action(view_menu, "fullscreen", this, SLOT(toggle_full_screen()), QKeySequence("F11"));
  full_screen->setCheckable(true);

  full_screen_viewer_ = MenuHelper::create_menu_action(view_menu, "fullscreenviewer", &olive::FocusFilter, SLOT(set_viewer_fullscreen()));

  // INITIALIZE PLAYBACK MENU

  playback_menu = MenuHelper::create_submenu(menuBar, this, SLOT(playbackMenu_About_To_Be_Shown()));

  go_to_start_ = MenuHelper::create_menu_action(playback_menu, "gotostart", &olive::FocusFilter, SLOT(go_to_start()), QKeySequence("Home"));
  previous_frame_ = MenuHelper::create_menu_action(playback_menu, "prevframe", &olive::FocusFilter, SLOT(prev_frame()), QKeySequence("Left"));
  playpause_ = MenuHelper::create_menu_action(playback_menu, "playpause", &olive::FocusFilter, SLOT(playpause()), QKeySequence("Space"));
  play_in_to_out_ = MenuHelper::create_menu_action(playback_menu, "playintoout", &olive::FocusFilter, SLOT(play_in_to_out()), QKeySequence("Shift+Space"));
  next_frame_ = MenuHelper::create_menu_action(playback_menu, "nextframe", &olive::FocusFilter, SLOT(next_frame()), QKeySequence("Right"));
  go_to_end_ = MenuHelper::create_menu_action(playback_menu, "gotoend", &olive::FocusFilter, SLOT(go_to_end()), QKeySequence("End"));

  playback_menu->addSeparator();

  go_to_prev_cut_ = MenuHelper::create_menu_action(playback_menu, "prevcut", panel_project.first(), SLOT(previous_cut()), QKeySequence("Up"));
  go_to_next_cut_ = MenuHelper::create_menu_action(playback_menu, "nextcut", panel_project.first(), SLOT(next_cut()), QKeySequence("Down"));

  playback_menu->addSeparator();

  go_to_in_point_ = MenuHelper::create_menu_action(playback_menu, "gotoin", &olive::FocusFilter, SLOT(go_to_in()), QKeySequence("Shift+I"));
  go_to_out_point_ = MenuHelper::create_menu_action(playback_menu, "gotoout", &olive::FocusFilter, SLOT(go_to_out()), QKeySequence("Shift+O"));

  playback_menu->addSeparator();

  shuttle_left_ = MenuHelper::create_menu_action(playback_menu, "decspeed", &olive::FocusFilter, SLOT(decrease_speed()), QKeySequence("J"));
  shuttle_stop_ = MenuHelper::create_menu_action(playback_menu, "pause", &olive::FocusFilter, SLOT(pause()), QKeySequence("K"));
  shuttle_right_ = MenuHelper::create_menu_action(playback_menu, "incspeed", &olive::FocusFilter, SLOT(increase_speed()), QKeySequence("L"));

  playback_menu->addSeparator();

  loop_action_ = MenuHelper::create_menu_action(playback_menu, "loop", &olive::MenuHelper, SLOT(toggle_bool_action()));
  loop_action_->setCheckable(true);
  loop_action_->setData(reinterpret_cast<quintptr>(&olive::config.loop));

  // INITIALIZE WINDOW MENU

  window_menu = MenuHelper::create_submenu(menuBar, this, SLOT(windowMenu_About_To_Be_Shown()));

  window_project_action = MenuHelper::create_menu_action(window_menu, "panelproject", this, SLOT(toggle_panel_visibility()));
  window_project_action->setCheckable(true);
  window_project_action->setData(reinterpret_cast<quintptr>(panel_project.first()));

  window_effectcontrols_action = MenuHelper::create_menu_action(window_menu, "paneleffectcontrols", this, SLOT(toggle_panel_visibility()));
  window_effectcontrols_action->setCheckable(true);
  window_effectcontrols_action->setData(reinterpret_cast<quintptr>(panel_effect_controls));

  window_timeline_action = MenuHelper::create_menu_action(window_menu, "paneltimeline", this, SLOT(toggle_panel_visibility()));
  window_timeline_action->setCheckable(true);
  window_timeline_action->setData(reinterpret_cast<quintptr>(panel_timeline.first()));

  window_graph_editor_action = MenuHelper::create_menu_action(window_menu, "panelgrapheditor", this, SLOT(toggle_panel_visibility()));
  window_graph_editor_action->setCheckable(true);
  window_graph_editor_action->setData(reinterpret_cast<quintptr>(panel_graph_editor));

  window_footageviewer_action = MenuHelper::create_menu_action(window_menu, "panelfootageviewer", this, SLOT(toggle_panel_visibility()));
  window_footageviewer_action->setCheckable(true);
  window_footageviewer_action->setData(reinterpret_cast<quintptr>(panel_footage_viewer));

  window_sequenceviewer_action = MenuHelper::create_menu_action(window_menu, "panelsequenceviewer", this, SLOT(toggle_panel_visibility()));
  window_sequenceviewer_action->setCheckable(true);
  window_sequenceviewer_action->setData(reinterpret_cast<quintptr>(panel_sequence_viewer));

  window_menu->addSeparator();

  maximize_panel_ = MenuHelper::create_menu_action(window_menu, "maximizepanel", this, SLOT(maximize_panel()), QKeySequence("`"));

  lock_panels_ = MenuHelper::create_menu_action(window_menu, "lockpanels", this, SLOT(set_panels_locked(bool)));
  lock_panels_->setCheckable(true);

  window_menu->addSeparator();

  reset_default_layout_ = MenuHelper::create_menu_action(window_menu, "resetdefaultlayout", this, SLOT(reset_layout()));

  // INITIALIZE TOOLS MENU

  tools_menu = MenuHelper::create_submenu(menuBar, this, SLOT(toolMenu_About_To_Be_Shown()));
  tools_menu->setToolTipsVisible(true);

  QActionGroup* tools_group = new QActionGroup(this);

  pointer_tool_action = MenuHelper::create_menu_action(tools_menu, "pointertool", &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("V"));
  pointer_tool_action->setCheckable(true);
  pointer_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline.first()->toolArrowButton));
  tools_group->addAction(pointer_tool_action);

  edit_tool_action = MenuHelper::create_menu_action(tools_menu, "edittool", &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("X"));
  edit_tool_action->setCheckable(true);
  edit_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline.first()->toolEditButton));
  tools_group->addAction(edit_tool_action);

  ripple_tool_action = MenuHelper::create_menu_action(tools_menu, "rippletool", &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("B"));
  ripple_tool_action->setCheckable(true);
  ripple_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline.first()->toolRippleButton));
  tools_group->addAction(ripple_tool_action);

  razor_tool_action = MenuHelper::create_menu_action(tools_menu, "razortool", &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("C"));
  razor_tool_action->setCheckable(true);
  razor_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline.first()->toolRazorButton));
  tools_group->addAction(razor_tool_action);

  slip_tool_action = MenuHelper::create_menu_action(tools_menu, "sliptool", &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("Y"));
  slip_tool_action->setCheckable(true);
  slip_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline.first()->toolSlipButton));
  tools_group->addAction(slip_tool_action);

  slide_tool_action = MenuHelper::create_menu_action(tools_menu, "slidetool", &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("U"));
  slide_tool_action->setCheckable(true);
  slide_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline.first()->toolSlideButton));
  tools_group->addAction(slide_tool_action);

  hand_tool_action = MenuHelper::create_menu_action(tools_menu, "handtool", &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("H"));
  hand_tool_action->setCheckable(true);
  hand_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline.first()->toolHandButton));
  tools_group->addAction(hand_tool_action);

  transition_tool_action = MenuHelper::create_menu_action(tools_menu, "transitiontool", &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("T"));
  transition_tool_action->setCheckable(true);
  transition_tool_action->setData(reinterpret_cast<quintptr>(panel_timeline.first()->toolTransitionButton));
  tools_group->addAction(transition_tool_action);

  tools_menu->addSeparator();

  snap_toggle = MenuHelper::create_menu_action(tools_menu, "snapping", &olive::MenuHelper, SLOT(menu_click_button()), QKeySequence("S"));
  snap_toggle->setCheckable(true);
  snap_toggle->setData(reinterpret_cast<quintptr>(panel_timeline.first()->snappingButton));

  tools_menu->addSeparator();

  autocut_silence_ = MenuHelper::create_menu_action(tools_menu, "autocutsilence", olive::Global.get(), SLOT(open_autocut_silence_dialog()));

  tools_menu->addSeparator();

  QActionGroup* autoscroll_group = new QActionGroup(this);

  no_autoscroll = MenuHelper::create_menu_action(tools_menu, "autoscrollno", &olive::MenuHelper, SLOT(set_autoscroll()));
  no_autoscroll->setData(olive::AUTOSCROLL_NO_SCROLL);
  no_autoscroll->setCheckable(true);
  autoscroll_group->addAction(no_autoscroll);

  page_autoscroll = MenuHelper::create_menu_action(tools_menu, "autoscrollpage", &olive::MenuHelper, SLOT(set_autoscroll()));
  page_autoscroll->setData(olive::AUTOSCROLL_PAGE_SCROLL);
  page_autoscroll->setCheckable(true);
  autoscroll_group->addAction(page_autoscroll);

  smooth_autoscroll = MenuHelper::create_menu_action(tools_menu, "autoscrollsmooth", &olive::MenuHelper, SLOT(set_autoscroll()));
  smooth_autoscroll->setData(olive::AUTOSCROLL_SMOOTH_SCROLL);
  smooth_autoscroll->setCheckable(true);
  autoscroll_group->addAction(smooth_autoscroll);

  tools_menu->addSeparator();

  preferences_action_ = MenuHelper::create_menu_action(tools_menu, "prefs", olive::Global.get(), SLOT(open_preferences()), QKeySequence("Ctrl+,"));

#ifdef QT_DEBUG
  clear_undo_action_ = MenuHelper::create_menu_action(tools_menu, "clearundo", olive::Global.get(), SLOT(clear_undo_stack()));
#endif

  // INITIALIZE HELP MENU

  help_menu = MenuHelper::create_submenu(menuBar);

  action_search_ = MenuHelper::create_menu_action(help_menu, "actionsearch", olive::Global.get(), SLOT(open_action_search()), QKeySequence("/"));

  help_menu->addSeparator();

  debug_log_ = MenuHelper::create_menu_action(help_menu, "debuglog", olive::Global.get(), SLOT(open_debug_log()));

  help_menu->addSeparator();

  about_action_ = MenuHelper::create_menu_action(help_menu, "about", olive::Global.get(), SLOT(open_about_dialog()));

  load_shortcuts(get_config_path() + "/shortcuts");
}

void MainWindow::Retranslate()
{
  file_menu->setTitle(tr("&File"));
  new_menu->setTitle(tr("&New"));
  open_project->setText(tr("&Open Project"));
  clear_open_recent_action->setText(tr("Clear Recent List"));
  open_recent->setTitle(tr("Open Recent"));
  save_project->setText(tr("&Save Project"));
  save_project_as->setText(tr("Save Project &As"));
  import_action->setText(tr("&Import..."));
  export_action->setText(tr("&Export..."));
  exit_action->setText(tr("E&xit"));

  edit_menu->setTitle(tr("&Edit"));
  undo_action->setText(tr("&Undo"));
  redo_action->setText(tr("Redo"));
  select_all_action->setText(tr("Select &All"));
  deselect_all_action->setText(tr("Deselect All"));
  ripple_to_in_point_->setText(tr("Ripple to In Point"));
  ripple_to_out_point_->setText(tr("Ripple to Out Point"));
  edit_to_in_point_->setText(tr("Edit to In Point"));
  edit_to_out_point_->setText(tr("Edit to Out Point"));
  delete_inout_point_->setText(tr("Delete In/Out Point"));
  ripple_delete_inout_point_->setText(tr("Ripple Delete In/Out Point"));
  setedit_marker_->setText(tr("Set/Edit Marker"));

  view_menu->setTitle(tr("&View"));
  zoom_in_->setText(tr("Zoom In"));
  zoom_out_->setText(tr("Zoom Out"));
  increase_track_height_->setText(tr("Increase Track Height"));
  decrease_track_height_->setText(tr("Decrease Track Height"));
  show_all->setText(tr("Toggle Show All"));
  rectified_waveforms->setText(tr("Rectified Waveforms"));
  frames_action->setText(tr("Frames"));
  drop_frame_action->setText(tr("Drop Frame"));
  nondrop_frame_action->setText(tr("Non-Drop Frame"));
  milliseconds_action->setText(tr("Milliseconds"));

  title_safe_area_menu->setTitle(tr("Title/Action Safe Area"));
  title_safe_off->setText(tr("Off"));
  title_safe_default->setText(tr("Default"));
  title_safe_43->setText(tr("4:3"));
  title_safe_169->setText(tr("16:9"));
  title_safe_custom->setText(tr("Custom"));

  full_screen->setText(tr("Full Screen"));
  full_screen_viewer_->setText(tr("Full Screen Viewer"));

  playback_menu->setTitle(tr("&Playback"));
  go_to_start_->setText(tr("Go to Start"));
  previous_frame_->setText(tr("Previous Frame"));
  playpause_->setText(tr("Play/Pause"));
  play_in_to_out_->setText(tr("Play In to Out"));
  next_frame_->setText(tr("Next Frame"));
  go_to_end_->setText(tr("Go to End"));

  go_to_prev_cut_->setText(tr("Go to Previous Cut"));
  go_to_next_cut_->setText(tr("Go to Next Cut"));
  go_to_in_point_->setText(tr("Go to In Point"));
  go_to_out_point_->setText(tr("Go to Out Point"));

  shuttle_left_->setText(tr("Shuttle Left"));
  shuttle_stop_->setText(tr("Shuttle Stop"));
  shuttle_right_->setText(tr("Shuttle Right"));

  loop_action_->setText(tr("Loop"));

  window_menu->setTitle(tr("&Window"));

  window_project_action->setText(tr("Project"));
  window_effectcontrols_action->setText(tr("Effect Controls"));
  window_timeline_action->setText(tr("Timeline"));
  window_graph_editor_action->setText(tr("Graph Editor"));
  window_footageviewer_action->setText(tr("Media Viewer"));
  window_sequenceviewer_action->setText(tr("Sequence Viewer"));

  maximize_panel_->setText(tr("Maximize Panel"));
  lock_panels_->setText(tr("Lock Panels"));
  reset_default_layout_->setText(tr("Reset to Default Layout"));

  tools_menu->setTitle(tr("&Tools"));

  pointer_tool_action->setText(tr("Pointer Tool"));
  edit_tool_action->setText(tr("Edit Tool"));
  ripple_tool_action->setText(tr("Ripple Tool"));
  razor_tool_action->setText(tr("Razor Tool"));
  slip_tool_action->setText(tr("Slip Tool"));
  slide_tool_action->setText(tr("Slide Tool"));
  hand_tool_action->setText(tr("Hand Tool"));
  transition_tool_action->setText(tr("Transition Tool"));
  snap_toggle->setText(tr("Enable Snapping"));
  autocut_silence_->setText(tr("Auto-Cut Silence"));

  no_autoscroll->setText(tr("No Auto-Scroll"));
  page_autoscroll->setText(tr("Page Auto-Scroll"));
  smooth_autoscroll->setText(tr("Smooth Auto-Scroll"));

  preferences_action_->setText(tr("Preferences"));
#ifdef QT_DEBUG
  clear_undo_action_->setText(tr("Clear Undo"));
#endif

  help_menu->setTitle(tr("&Help"));

  action_search_->setText(tr("A&ction Search"));
  debug_log_->setText(tr("Debug Log"));
  about_action_->setText(tr("&About..."));

  panel_sequence_viewer->set_panel_name(QCoreApplication::translate("Viewer", "Sequence Viewer"));
  panel_footage_viewer->set_panel_name(QCoreApplication::translate("Viewer", "Media Viewer"));

  // the recommended changeEvent() and event() methods of propagating language change messages provided mixed results
  // (i.e. different panels failed to translate in different sessions), so we translate them manually here
  for (int i=0;i<olive::panels.size();i++) {
    olive::panels.at(i)->Retranslate();
  }
  olive::MenuHelper.Retranslate();

  updateTitle();
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
    olive::proxy_generator.cancel();

    panel_graph_editor->set_row(nullptr);
    panel_effect_controls->Clear(true);

    panel_footage_viewer->viewer_widget()->close_window();
    panel_sequence_viewer->viewer_widget()->close_window();

    olive::undo_stack.clear();

    QString data_dir = get_data_path();
    QString config_path = get_config_path();

    const QString& autorecovery_filename = olive::Global->get_autorecovery_filename();
    if (!data_dir.isEmpty() && !autorecovery_filename.isEmpty()) {
      if (QFile::exists(autorecovery_filename)) {
        QFile::rename(autorecovery_filename,
                      autorecovery_filename + "." + QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmss"));
      }
    }
    if (!config_path.isEmpty()) {
      QDir config_dir = QDir(config_path);

      QString config_fn = config_dir.filePath("config.xml");

      // save settings
      olive::config.save(config_fn);

      // save panel layout
      QFile panel_config(get_config_dir().filePath("layout"));
      if (panel_config.open(QFile::WriteOnly)) {
        QXmlStreamWriter stream(&panel_config);
        stream.setAutoFormatting(true);
        stream.writeStartDocument();

        stream.writeStartElement("layout");

        stream.writeTextElement("panels", saveState(0).toBase64());

        // if the panels have any specific layout data to save, save it now
        for (int i=0;i<olive::panels.size();i++) {
          QByteArray layout_data = olive::panels.at(i)->SaveLayoutState();

          if (!layout_data.isEmpty()) {

            // layout data is matched with the panel's objectName(), which we can't do if the panel has no name
            const QString& panel_name = olive::panels.at(i)->objectName();
            if (panel_name.isEmpty()) {
              qWarning() << "Panel" << i << "had layout state data but no objectName(). Layout was not saved.";
            } else {
              stream.writeStartElement("panel");

              stream.writeAttribute("name", panel_name);

              stream.writeCharacters(layout_data.toBase64());

              stream.writeEndElement();
            }
          }
        }

        stream.writeEndElement(); // layout

        stream.writeEndDocument();
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
    // set this to false immediately to prevent anything here being called again
    first_show = false;

    /**
     * @brief Set up the dock widget layout on the main window
     *
     * For some reason, Qt didn't like this in the constructor. It would lead to several geometry issues with HiDPI
     * on Windows, and also seemed to break QMainWindow::restoreState() which is why it took so long to implement
     * saving/restoring panel layouts. Putting it in showEvent() didn't help either, nor did putting it in
     * changeEvent() (QEvent::type() == QEvent::Polish). This is the only place it's functioned as expected.
     */
    setup_layout(false);

    /**
      Signal that window has finished loading.
     */
    emit finished_first_paint();
  }
}

void MainWindow::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {

    // if this was a LanguageEvent, run the retranslation function
    Retranslate();

  } else {

    // otherwise pass it to the base class
    QMainWindow::changeEvent(e);

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

      // remove all dock widgets that aren't the hovered panel
      for (int i=0;i<olive::panels.size();i++) {
        if (olive::panels.at(i) != focused_panel) {
          // hide the panel
          olive::panels.at(i)->setVisible(false);

          // set it to floating
          olive::panels.at(i)->setFloating(true);
        }
      }
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
  olive::MenuHelper.set_bool_action_checked(loop_action_);
}

void MainWindow::viewMenu_About_To_Be_Shown() {
  olive::MenuHelper.set_bool_action_checked(rectified_waveforms);

  olive::MenuHelper.set_int_action_checked(frames_action, olive::config.timecode_view);
  olive::MenuHelper.set_int_action_checked(drop_frame_action, olive::config.timecode_view);
  olive::MenuHelper.set_int_action_checked(nondrop_frame_action, olive::config.timecode_view);
  olive::MenuHelper.set_int_action_checked(milliseconds_action, olive::config.timecode_view);

  title_safe_off->setChecked(!olive::config.show_title_safe_area);
  title_safe_default->setChecked(olive::config.show_title_safe_area
                                 && !olive::config.use_custom_title_safe_ratio);
  title_safe_43->setChecked(olive::config.show_title_safe_area
                            && olive::config.use_custom_title_safe_ratio
                            && qFuzzyCompare(olive::config.custom_title_safe_ratio, title_safe_43->data().toDouble()));
  title_safe_169->setChecked(olive::config.show_title_safe_area
                             && olive::config.use_custom_title_safe_ratio
                             && qFuzzyCompare(olive::config.custom_title_safe_ratio, title_safe_169->data().toDouble()));
  title_safe_custom->setChecked(olive::config.show_title_safe_area
                                && olive::config.use_custom_title_safe_ratio
                                && !title_safe_43->isChecked()
                                && !title_safe_169->isChecked());

  full_screen->setChecked(windowState() == Qt::WindowFullScreen);

  show_all->setChecked(panel_timeline.first()->showing_all);
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

  olive::MenuHelper.set_int_action_checked(no_autoscroll, olive::config.autoscroll);
  olive::MenuHelper.set_int_action_checked(page_autoscroll, olive::config.autoscroll);
  olive::MenuHelper.set_int_action_checked(smooth_autoscroll, olive::config.autoscroll);
}

void MainWindow::toggle_panel_visibility() {
  QAction* action = static_cast<QAction*>(sender());
  QDockWidget* w = reinterpret_cast<QDockWidget*>(action->data().value<quintptr>());
  w->setVisible(!w->isVisible());

  // layout has changed, we're no longer in maximized panel mode,
  // so we clear this byte array
  temp_panel_state.clear();
}

void MainWindow::set_panels_locked(bool locked)
{
  for (int i=0;i<olive::panels.size();i++) {
    Panel* panel = olive::panels.at(i);

    if (locked) {
      // disable moving on QDockWidget
      panel->setFeatures(panel->features() & ~QDockWidget::DockWidgetMovable);

      // hide the title bar (only real way to do this is to replace it with an empty QWidget)
      panel->setTitleBarWidget(new QWidget(panel));
    } else {
      // re-enable moving on QDockWidget
      panel->setFeatures(panel->features() | QDockWidget::DockWidgetMovable);

      // set the "custom" titlebar to null so the default gets restored
      panel->setTitleBarWidget(nullptr);
    }
  }
}

void MainWindow::fileMenu_About_To_Be_Shown() {
  if (olive::Global->recent_project_count() > 0) {
    open_recent->clear();
    open_recent->setEnabled(true);
    for (int i=0;i<olive::Global->recent_project_count();i++) {
      QAction* action = open_recent->addAction(olive::Global->recent_project(i));
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
