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
#include "global/timing.h"
#include "global/clipboard.h"
#include "rendering/audio.h"
#include "dialogs/demonotice.h"
#include "dialogs/preferencesdialog.h"
#include "dialogs/exportdialog.h"
#include "dialogs/debugdialog.h"
#include "dialogs/aboutdialog.h"
#include "dialogs/speeddialog.h"
#include "dialogs/actionsearch.h"
#include "dialogs/newsequencedialog.h"
#include "dialogs/loaddialog.h"
#include "dialogs/autocutsilencedialog.h"
#include "project/loadthread.h"
#include "project/savethread.h"
#include "timeline/sequence.h"
#include "ui/mediaiconservice.h"
#include "ui/mainwindow.h"
#include "ui/updatenotification.h"
#include "undo/undostack.h"

std::unique_ptr<OliveGlobal> olive::Global;
QString olive::ActiveProjectFilename;
QString olive::AppName;

OliveGlobal::OliveGlobal() :
  changed_since_last_autorecovery(false),
  rendering_(false)
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

bool OliveGlobal::is_exporting()
{
  return rendering_;
}

void OliveGlobal::set_export_state(bool rendering) {
  rendering_ = rendering;
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
  QString language_file = olive::runtime_config.external_translation_file.isEmpty() ?
        olive::config.language_file :
        olive::runtime_config.external_translation_file;

  // clear runtime language file so if the user sets a different language, we won't load it next time
  olive::runtime_config.external_translation_file.clear();

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
#else
  Q_UNUSED(w)
#endif
}

void OliveGlobal::add_recent_project(const QString &url)
{
  bool found = false;
  for (int i=0;i<recent_projects.size();i++) {
    if (url == recent_projects.at(i)) {
      found = true;
      recent_projects.move(i, 0);
      break;
    }
  }
  if (!found) {
    recent_projects.prepend(url);
    if (recent_projects.size() > olive::config.maximum_recent_projects) {
      recent_projects.removeLast();
    }
  }
  save_recent_projects();
}

void OliveGlobal::load_recent_projects()
{
  QFile f(get_recent_project_list_file());
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

int OliveGlobal::recent_project_count()
{
  return recent_projects.size();
}

const QString &OliveGlobal::recent_project(int index)
{
  return recent_projects.at(index);
}

const QString &OliveGlobal::get_autorecovery_filename()
{
  return autorecovery_filename;
}

void OliveGlobal::LoadProject(const QString &fn, bool autorecovery)
{
  // QSortFilterProxyModels are not thread-safe, and as we'll be loading in another thread, leaving it connected
  // can cause glitches in its presentation. Therefore for the duration of the loading process, we disconnect it,
  // and reconnect it later once the loading is complete.

  for (int i=0;i<panel_project.size();i++) {
    panel_project.at(i)->DisconnectFilterToModel();
  }

  LoadDialog ld(olive::MainWindow);

  ld.open();

  LoadThread* lt = new LoadThread(fn, autorecovery);
  connect(&ld, SIGNAL(cancel()), lt, SLOT(cancel()));
  connect(lt, SIGNAL(success()), &ld, SLOT(accept()));
  connect(lt, SIGNAL(error()), &ld, SLOT(reject()));
  connect(lt, SIGNAL(error()), this, SLOT(new_project()));
  connect(lt, SIGNAL(report_progress(int)), &ld, SLOT(setValue(int)));
  lt->start();

  for (int i=0;i<panel_project.size();i++) {
    panel_project.at(i)->ConnectFilterToModel();
  }
}

void OliveGlobal::ClearProject()
{
  // clear graph editor
  panel_graph_editor->set_row(nullptr);

  // clear effects panel
  panel_effect_controls->Clear(true);

  // clear existing project
  Timeline::CloseAll();
  panel_footage_viewer->set_media(nullptr);

  // delete sequences first because it's important to close all the clips before deleting the media
  QVector<Media*> sequences = olive::project_model.GetAllSequences();
  for (int i=0;i<sequences.size();i++) {
    sequences.at(i)->set_sequence(nullptr);
  }

  // clear project contents (footage, sequences, etc.)
  olive::project_model.clear();

  // clear undo stack
  olive::undo_stack.clear();

  // empty current project filename
  update_project_filename("");

  // full update of all panels
  update_ui(false);

  // set to unmodified
  set_modified(false);
}

void OliveGlobal::save_recent_projects()
{
  // save to file
  QFile f(get_recent_project_list_file());
  if (f.open(QFile::WriteOnly | QFile::Truncate | QFile::Text)) {
    QTextStream out(&f);
    for (int i=0;i<recent_projects.size();i++) {
      if (i > 0) {
        out << "\n";
      }
      out << recent_projects.at(i);
    }
    f.close();
  } else {
    qWarning() << "Could not save recent projects";
  }
}

void OliveGlobal::PasteInternal(Sequence *s, bool insert)
{
  if (s == nullptr) {
    return;
  }

  if (!olive::clipboard.IsEmpty()) {
    if (olive::clipboard.type() == Clipboard::CLIPBOARD_TYPE_CLIP) {
      ComboAction* ca = new ComboAction();

      // create copies and delete areas that we'll be pasting to
      QVector<Selection> delete_areas;
      QVector<Clip*> original_clips;
      QVector<ClipPtr> pasted_clips;
      long paste_start = LONG_MAX;
      long paste_end = LONG_MIN;

      for (int i=0;i<olive::clipboard.Count();i++) {
        ClipPtr c = std::static_pointer_cast<Clip>(olive::clipboard.Get(i));

        // create copy of clip and offset by playhead
        ClipPtr cc = c->copy(s->GetTrackList(c->track()->type())->TrackAt(c->track()->Index()));

        // convert frame rates
        cc->set_timeline_in(rescale_frame_number(cc->timeline_in(), c->cached_frame_rate(), s->frame_rate));
        cc->set_timeline_out(rescale_frame_number(cc->timeline_out(), c->cached_frame_rate(), s->frame_rate));
        cc->set_clip_in(rescale_frame_number(cc->clip_in(), c->cached_frame_rate(), s->frame_rate));

        cc->set_timeline_in(cc->timeline_in() + s->playhead);
        cc->set_timeline_out(cc->timeline_out() + s->playhead);
        cc->set_track(c->track());

        paste_start = qMin(paste_start, cc->timeline_in());
        paste_end = qMax(paste_end, cc->timeline_out());

        original_clips.append(c.get());
        pasted_clips.append(cc);

        if (!insert) {
          delete_areas.append(Selection(cc->timeline_in(), cc->timeline_out(), c->track()));
        }
      }
      if (insert) {
        s->SplitAllClipsAtPoint(ca, s->playhead);
        s->Ripple(ca, paste_start, paste_end - paste_start);
      } else {
        s->DeleteAreas(ca, delete_areas, false);
      }

      // correct linked clips
      olive::timeline::RelinkClips(original_clips, pasted_clips);

      ca->append(new AddClipCommand(pasted_clips));

      olive::undo_stack.push(ca);

      update_ui(true);

      if (olive::config.paste_seeks) {
        panel_sequence_viewer->seek(paste_end);
      }

    } else if (olive::clipboard.type() == Clipboard::CLIPBOARD_TYPE_EFFECT) {
      ComboAction* ca = new ComboAction();

      bool replace = false;
      bool skip = false;
      bool ask_conflict = true;

      QVector<Clip*> selected_clips = s->SelectedClips();

      for (int i=0;i<selected_clips.size();i++) {
        Clip* c = selected_clips.at(i);

        for (int j=0;j<olive::clipboard.Count();j++) {
          NodePtr e = std::static_pointer_cast<Node>(olive::clipboard.Get(j));
          if (c->type() == e->subtype()) {
            int found = -1;
            if (ask_conflict) {
              replace = false;
              skip = false;
            }
            for (int k=0;k<c->effects.size();k++) {
              if (c->effects.at(k)->id() == e->id()) {
                found = k;
                break;
              }
            }
            if (found >= 0 && ask_conflict) {
              QMessageBox box(olive::MainWindow);
              box.setWindowTitle(tr("Effect already exists"));
              box.setText(tr("Clip '%1' already contains a '%2' effect. "
                             "Would you like to replace it with the pasted one or add it as a separate effect?")
                          .arg(c->name(), e->name()));
              box.setIcon(QMessageBox::Icon::Question);

              box.addButton(tr("Add"), QMessageBox::YesRole);
              QPushButton* replace_button = box.addButton(tr("Replace"), QMessageBox::NoRole);
              QPushButton* skip_button = box.addButton(tr("Skip"), QMessageBox::RejectRole);

              QCheckBox* future_box = new QCheckBox(tr("Do this for all conflicts found"), &box);
              box.setCheckBox(future_box);

              box.exec();

              if (box.clickedButton() == replace_button) {
                replace = true;
              } else if (box.clickedButton() == skip_button) {
                skip = true;
              }
              ask_conflict = !future_box->isChecked();
            }

            if (found >= 0 && skip) {
              // do nothing
            } else if (found >= 0 && replace) {
              ca->append(new EffectDeleteCommand(c->effects.at(found).get()));

              ca->append(new AddEffectCommand(c, e->copy(c), kInvalidNode, found));
            } else {
              ca->append(new AddEffectCommand(c, e->copy(c), kInvalidNode));
            }
          }
        }
      }
      if (ca->hasActions()) {
        ca->appendPost(new ReloadEffectsCommand());
        olive::undo_stack.push(ca);
      } else {
        delete ca;
      }
      update_ui(true);
    }
  }
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
      save_recent_projects();
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
    olive::Save(false);
    return true;
  }
  return false;
}

bool OliveGlobal::save_project() {
  if (olive::ActiveProjectFilename.isEmpty()) {
    return save_project_as();
  } else {
    olive::Save(false);
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

void OliveGlobal::open_new_sequence_dialog()
{
  NewSequenceDialog nsd(olive::MainWindow);
  nsd.set_sequence_name(olive::project_model.GetNextSequenceName());
  nsd.exec();
}

void OliveGlobal::open_import_dialog()
{
  QFileDialog fd(olive::MainWindow, tr("Import media..."), "", tr("All Files") + " (*)");
  fd.setFileMode(QFileDialog::ExistingFiles);

  if (fd.exec()) {
    QStringList files = fd.selectedFiles();

    Media* parent = nullptr;
    for (int i=0;i<panel_project.size();i++) {
      if (panel_project.at(i)->focused()) {
        parent = panel_project.at(i)->get_selected_folder();
        break;
      }
    }

    olive::project_model.process_file_list(files, false, nullptr, parent);
  }
}

void OliveGlobal::open_export_dialog() {
  if (CheckForActiveSequence()) {
    ExportDialog e(olive::MainWindow, Timeline::GetTopSequence().get());
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
    olive::Save(true);

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

void OliveGlobal::PrimarySequenceChanged()
{
  panel_graph_editor->set_row(nullptr);
  panel_effect_controls->Clear(true);
}

void OliveGlobal::clear_recent_projects()
{
  recent_projects.clear();
  save_recent_projects();
}

void OliveGlobal::OpenProjectWorker(QString fn, bool autorecovery) {
  ClearProject();
  update_project_filename(fn);
  LoadProject(fn, autorecovery);
  olive::undo_stack.clear();
}

bool OliveGlobal::CheckForActiveSequence(bool show_msg)
{
  if (Timeline::GetTopSequence()  == nullptr) {

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
  if (!Timeline::IsImporting()) {
    olive::undo_stack.undo();
    update_ui(true);
  }
}

void OliveGlobal::redo() {
  // workaround to prevent crash (and also users should never need to do this)
  if (!Timeline::IsImporting()) {
    olive::undo_stack.redo();
    update_ui(true);
  }
}

void OliveGlobal::paste() {
  PasteInternal(Timeline::GetTopSequence().get(), false);
}

void OliveGlobal::paste_insert() {
  PasteInternal(Timeline::GetTopSequence().get(), true);
}

void OliveGlobal::open_about_dialog() {
  AboutDialog a(olive::MainWindow);
  a.exec();
}

void OliveGlobal::open_debug_log() {
  olive::DebugDialog->show();
}

void OliveGlobal::open_speed_dialog() {
  if (Timeline::GetTopSequence() != nullptr) {

    QVector<Clip*> selected_clips = Timeline::GetTopSequence()->SelectedClips();

    if (!selected_clips.isEmpty()) {
      SpeedDialog s(olive::MainWindow, selected_clips);
      s.exec();
    }
  }
}

void OliveGlobal::open_autocut_silence_dialog() {
  if (CheckForActiveSequence()) {

    QVector<Clip*> selected_clips = Timeline::GetTopSequence()->SelectedClips();

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
  olive::undo_stack.clear();
}

void OliveGlobal::open_action_search() {
  ActionSearch as(olive::MainWindow);
  as.exec();
}
