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

#include "global/global.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QAction>
#include <QApplication>
#include <QStyleFactory>
#include <QDebug>

#include "panels/panels.h"
#include "global/path.h"
#include "global/config.h"
#include "rendering/audio.h"
#include "dialogs/demonotice.h"
#include "dialogs/preferencesdialog.h"
#include "dialogs/exportdialog.h"
#include "dialogs/debugdialog.h"
#include "dialogs/aboutdialog.h"
#include "dialogs/speeddialog.h"
#include "dialogs/actionsearch.h"
#include "dialogs/loaddialog.h"
#include "dialogs/autocutsilencedialog.h"
#include "project/loadthread.h"
#include "timeline/sequence.h"
#include "ui/mediaiconservice.h"
#include "ui/mainwindow.h"
#include "ui/updatenotification.h"
#include "undo/undostack.h"

std::unique_ptr<OliveGlobal> olive::Global;
QString olive::ActiveProjectFilename;
QString olive::AppName;

OliveGlobal::OliveGlobal() :
  changed_since_last_autorecovery(false)
{
  // sets current app name
  QString version_id;

  // if available, append the current Git hash (defined by `qmake` and the Makefile)
#ifdef GITHASH
  version_id = QString(" | %1").arg(GITHASH);
#endif

  olive::AppName = QString("Olive (March 2019 | Alpha%1)").arg(version_id);

  // set the file filter used in all file dialogs pertaining to Olive project files.
  project_file_filter = tr("Olive Project %1").arg("(*.ove)");

  // set default value
  enable_load_project_on_init = false;

  // alloc QTranslator
  translator = std::unique_ptr<QTranslator>(new QTranslator());
}

const QString &OliveGlobal::get_project_file_filter() {
  return project_file_filter;
}

void OliveGlobal::update_project_filename(const QString &s) {
  // set filename to s
  olive::ActiveProjectFilename = s;

  // update main window title to reflect new project filename
  olive::MainWindow->updateTitle();
}

void OliveGlobal::check_for_autorecovery_file() {
  QString data_dir = get_data_path();
  if (!data_dir.isEmpty()) {
    // detect auto-recovery file
    autorecovery_filename = data_dir + "/autorecovery.ove";
    if (QFile::exists(autorecovery_filename)) {
      if (QMessageBox::question(nullptr,
                                tr("Auto-recovery"),
                                tr("Olive didn't close properly and an autorecovery file "
                                   "was detected. Would you like to open it?"),
                                QMessageBox::Yes,
                                QMessageBox::No) == QMessageBox::Yes) {
        enable_load_project_on_init = false;
        OpenProjectWorker(autorecovery_filename, true);
      }
    }
    autorecovery_timer.setInterval(60000);
    QObject::connect(&autorecovery_timer, SIGNAL(timeout()), this, SLOT(save_autorecovery_file()));
    autorecovery_timer.start();
  }
}

void OliveGlobal::set_rendering_state(bool rendering) {
  audio_rendering = rendering;
  if (rendering) {
    autorecovery_timer.stop();
  } else {
    autorecovery_timer.start();
  }
}

void OliveGlobal::set_modified(bool modified)
{
  olive::MainWindow->setWindowModified(modified);
  changed_since_last_autorecovery = modified;
}

bool OliveGlobal::is_modified()
{
  return olive::MainWindow->isWindowModified();
}

void OliveGlobal::load_project_on_launch(const QString& s) {
  olive::ActiveProjectFilename = s;
  enable_load_project_on_init = true;
}

QString OliveGlobal::get_recent_project_list_file() {
  return get_data_dir().filePath("recents");
}

void OliveGlobal::load_translation_from_config() {
  QString language_file = olive::CurrentRuntimeConfig.external_translation_file.isEmpty() ?
        olive::CurrentConfig.language_file :
        olive::CurrentRuntimeConfig.external_translation_file;

  // clear runtime language file so if the user sets a different language, we won't load it next time
  olive::CurrentRuntimeConfig.external_translation_file.clear();

  // remove current translation if there is one
  QApplication::removeTranslator(translator.get());

  if (!language_file.isEmpty()) {

    // translation files are stored relative to app path (see GitHub issue #454)
    QString full_language_path = QDir(get_app_path()).filePath(language_file);

    // load translation file
    if (QFileInfo::exists(full_language_path)
        && translator->load(full_language_path)) {
      QApplication::installTranslator(translator.get());
    } else {
      qWarning() << "Failed to load translation file" << full_language_path << ". No language will be loaded.";
    }
  }
}

void OliveGlobal::SetNativeStyling(QWidget *w)
{
#ifdef Q_OS_WIN
  w->setStyleSheet("");
  w->setPalette(w->style()->standardPalette());
  w->setStyle(QStyleFactory::create("windowsvista"));
#endif
}

void OliveGlobal::LoadProject(const QString &fn, bool autorecovery)
{
  // QSortFilterProxyModels are not thread-safe, and as we'll be loading in another thread, leaving it connected
  // can cause glitches in its presentation. Therefore for the duration of the loading process, we disconnect it,
  // and reconnect it later once the loading is complete.

  panel_project->DisconnectFilterToModel();

  LoadDialog ld(olive::MainWindow);

  ld.open();

  LoadThread* lt = new LoadThread(fn, autorecovery);
  connect(&ld, SIGNAL(cancel()), lt, SLOT(cancel()));
  connect(lt, SIGNAL(success()), &ld, SLOT(accept()));
  connect(lt, SIGNAL(error()), &ld, SLOT(reject()));
  connect(lt, SIGNAL(error()), this, SLOT(new_project()));
  connect(lt, SIGNAL(report_progress(int)), &ld, SLOT(setValue(int)));
  lt->start();

  panel_project->ConnectFilterToModel();
}

void OliveGlobal::ClearProject()
{
  // clear graph editor
  panel_graph_editor->set_row(nullptr);

  // clear effects panel
  panel_effect_controls->Clear(true);

  // clear existing project
  olive::Global->set_sequence(nullptr);
  panel_footage_viewer->set_media(nullptr);

  // clear project contents (footage, sequences, etc.)
  panel_project->clear();

  // clear undo stack
  olive::UndoStack.clear();

  // empty current project filename
  update_project_filename("");

  // full update of all panels
  update_ui(false);

  // set to unmodified
  olive::Global->set_modified(false);
}

void OliveGlobal::ImportProject(const QString &fn)
{
  LoadProject(fn, false);
  set_modified(true);
}

void OliveGlobal::new_project() {
  if (can_close_project()) {
    ClearProject();
  }
}

void OliveGlobal::OpenProject() {
  QString fn = QFileDialog::getOpenFileName(olive::MainWindow, tr("Open Project..."), "", project_file_filter);
  if (!fn.isEmpty() && can_close_project()) {
    OpenProjectWorker(fn, false);
  }
}

void OliveGlobal::open_recent(int index) {
  QString recent_url = recent_projects.at(index);
  if (!QFile::exists(recent_url)) {
    if (QMessageBox::question(
          olive::MainWindow,
          tr("Missing recent project"),
          tr("The project '%1' no longer exists. Would you like to remove it from the recent projects list?").arg(recent_url),
          QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
      recent_projects.removeAt(index);
      panel_project->save_recent_projects();
    }
  } else if (can_close_project()) {
    OpenProjectWorker(recent_url, false);
  }
}

bool OliveGlobal::save_project_as() {
  QString fn = QFileDialog::getSaveFileName(olive::MainWindow, tr("Save Project As..."), "", project_file_filter);
  if (!fn.isEmpty()) {
    if (!fn.endsWith(".ove", Qt::CaseInsensitive)) {
      fn += ".ove";
    }
    update_project_filename(fn);
    panel_project->save_project(false);
    return true;
  }
  return false;
}

bool OliveGlobal::save_project() {
  if (olive::ActiveProjectFilename.isEmpty()) {
    return save_project_as();
  } else {
    panel_project->save_project(false);
    return true;
  }
}

bool OliveGlobal::can_close_project() {
  if (is_modified()) {
    QMessageBox* m = new QMessageBox(
          QMessageBox::Question,
          tr("Unsaved Project"),
          tr("This project has changed since it was last saved. Would you like to save it before closing?"),
          QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,
          olive::MainWindow
          );
    m->setWindowModality(Qt::WindowModal);
    int r = m->exec();
    delete m;
    if (r == QMessageBox::Yes) {
      return save_project();
    } else if (r == QMessageBox::Cancel) {
      return false;
    }
  }
  return true;
}

void OliveGlobal::open_export_dialog() {
  if (CheckForActiveSequence()) {
    ExportDialog e(olive::MainWindow);
    e.exec();
  }
}

void OliveGlobal::finished_initialize() {
  if (enable_load_project_on_init) {

    // if a project was set as a command line argument, we load it here
    if (QFileInfo::exists(olive::ActiveProjectFilename)) {
      OpenProjectWorker(olive::ActiveProjectFilename, false);
    } else {
      QMessageBox::critical(olive::MainWindow,
                            tr("Missing Project File"),
                            tr("Specified project '%1' does not exist.").arg(olive::ActiveProjectFilename),
                            QMessageBox::Ok);
      update_project_filename(nullptr);
    }

    enable_load_project_on_init = false;

  } else {
    // if we are not loading a project on launch and are running a release build, open the demo notice dialog
#ifndef QT_DEBUG
    DemoNotice* d = new DemoNotice(olive::MainWindow);
    connect(d, SIGNAL(finished(int)), d, SLOT(deleteLater()));
    d->open();
#endif
  }

  // Check for updates
  olive::update_notifier.check();
}

void OliveGlobal::save_autorecovery_file() {
  if (changed_since_last_autorecovery) {
    panel_project->save_project(true);

    changed_since_last_autorecovery = false;

    qInfo() << "Auto-recovery project saved";
  }
}

void OliveGlobal::open_preferences() {
  panel_sequence_viewer->pause();
  panel_footage_viewer->pause();

  PreferencesDialog pd(olive::MainWindow);
  pd.exec();
}

void OliveGlobal::set_sequence(SequencePtr s)
{
  panel_graph_editor->set_row(nullptr);
  panel_effect_controls->Clear(true);

  olive::ActiveSequence = s;
  panel_sequence_viewer->set_main_sequence();
  panel_timeline->update_sequence();
  panel_timeline->setFocus();
}

void OliveGlobal::OpenProjectWorker(QString fn, bool autorecovery) {
  ClearProject();
  update_project_filename(fn);
  LoadProject(fn, autorecovery);
  olive::UndoStack.clear();
}

bool OliveGlobal::CheckForActiveSequence(bool show_msg)
{
  if (olive::ActiveSequence == nullptr) {

    if (show_msg) {
      QMessageBox::information(olive::MainWindow,
                               tr("No active sequence"),
                               tr("Please open the sequence to perform this action."),
                               QMessageBox::Ok);
    }

    return false;
  }
  return true;
}

void OliveGlobal::undo() {
  // workaround to prevent crash (and also users should never need to do this)
  if (!panel_timeline->importing) {
    olive::UndoStack.undo();
    update_ui(true);
  }
}

void OliveGlobal::redo() {
  // workaround to prevent crash (and also users should never need to do this)
  if (!panel_timeline->importing) {
    olive::UndoStack.redo();
    update_ui(true);
  }
}

void OliveGlobal::paste() {
  if (olive::ActiveSequence != nullptr) {
    panel_timeline->paste(false);
  }
}

void OliveGlobal::paste_insert() {
  if (olive::ActiveSequence != nullptr) {
    panel_timeline->paste(true);
  }
}

void OliveGlobal::open_about_dialog() {
  AboutDialog a(olive::MainWindow);
  a.exec();
}

void OliveGlobal::open_debug_log() {
  olive::DebugDialog->show();
}

void OliveGlobal::open_speed_dialog() {
  if (olive::ActiveSequence != nullptr) {

    QVector<Clip*> selected_clips = olive::ActiveSequence->SelectedClips();

    if (!selected_clips.isEmpty()) {
      SpeedDialog s(olive::MainWindow, selected_clips);
      s.exec();
    }
  }
}

void OliveGlobal::open_autocut_silence_dialog() {
  if (CheckForActiveSequence()) {

    QVector<Clip*> selected_clips = olive::ActiveSequence->SelectedClips();

    if (selected_clips.isEmpty()) {
      QMessageBox::critical(olive::MainWindow,
                            tr("No clips selected"),
                            tr("Select the clips you wish to auto-cut"),
                            QMessageBox::Ok);
    } else {
      AutoCutSilenceDialog s(olive::MainWindow, selected_clips);
      s.exec();
    }

  }
}

void OliveGlobal::clear_undo_stack() {
  olive::UndoStack.clear();
}

void OliveGlobal::open_action_search() {
  ActionSearch as(olive::MainWindow);
  as.exec();
}
