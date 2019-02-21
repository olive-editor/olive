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

#include "oliveglobal.h"

#include "mainwindow.h"

#include "panels/panels.h"

#include "io/path.h"
#include "io/config.h"

#include "playback/audio.h"

#include "dialogs/demonotice.h"
#include "dialogs/preferencesdialog.h"
#include "dialogs/exportdialog.h"
#include "dialogs/debugdialog.h"
#include "dialogs/aboutdialog.h"
#include "dialogs/speeddialog.h"
#include "dialogs/actionsearch.h"

#include "project/sequence.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QAction>
#include <QApplication>
#include <QDebug>

std::unique_ptr<OliveGlobal> olive::Global;
QString olive::ActiveProjectFilename;
QString olive::AppName;

OliveGlobal::OliveGlobal() {
    // sets current app name
    QString version_id;
#ifdef GITHASH
    version_id = QString(" | %1").arg(GITHASH);
#endif
    olive::AppName = QString("Olive (February 2019 | Alpha%1)").arg(version_id);

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
            if (QMessageBox::question(nullptr, tr("Auto-recovery"), tr("Olive didn't close properly and an autorecovery file was detected. Would you like to open it?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
                enable_load_project_on_init = false;
                open_project_worker(autorecovery_filename, true);
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

void OliveGlobal::new_project() {
    if (can_close_project()) {
        // clear effects panel
        panel_effect_controls->clear_effects(true);

        // clear project contents (footage, sequences, etc.)
        panel_project->new_project();

        // clear undo stack
        olive::UndoStack.clear();

        // empty current project filename
        update_project_filename("");

        // full update of all panels
        update_ui(false);
    }
}

void OliveGlobal::open_project() {
    QString fn = QFileDialog::getOpenFileName(olive::MainWindow, tr("Open Project..."), "", project_file_filter);
    if (!fn.isEmpty() && can_close_project()) {
        open_project_worker(fn, false);
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
        open_project_worker(recent_url, false);
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
    if (olive::MainWindow->isWindowModified()) {
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
    if (olive::ActiveSequence == nullptr) {
        QMessageBox::information(olive::MainWindow,
                                 tr("No active sequence"),
                                 tr("Please open the sequence you wish to export."),
                                 QMessageBox::Ok);
    } else {
        ExportDialog e(olive::MainWindow);
        e.exec();
    }
}

void OliveGlobal::finished_initialize() {
    if (enable_load_project_on_init) {

        // if a project was set as a command line argument, we load it here
        if (QFileInfo::exists(olive::ActiveProjectFilename)) {
            open_project_worker(olive::ActiveProjectFilename, false);
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
}

void OliveGlobal::save_autorecovery_file() {
    if (olive::MainWindow->isWindowModified()) {
        panel_project->save_project(true);
        qInfo() << "Auto-recovery project saved";
    }
}

void OliveGlobal::open_preferences() {
    panel_sequence_viewer->pause();
    panel_footage_viewer->pause();

    PreferencesDialog pd(olive::MainWindow);
    pd.setup_kbd_shortcuts(olive::MainWindow->menuBar());
    pd.exec();
}

void OliveGlobal::open_project_worker(const QString& fn, bool autorecovery) {
    update_project_filename(fn);
    panel_project->load_project(autorecovery);
    olive::UndoStack.clear();
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
        SpeedDialog s(olive::MainWindow);
        for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
            ClipPtr c = olive::ActiveSequence->clips.at(i);
            if (c != nullptr && is_clip_selected(c, true)) {
                s.clips.append(c);
            }
        }
        if (s.clips.size() > 0) s.run();
    }
}

void OliveGlobal::clear_undo_stack() {
    olive::UndoStack.clear();
}

void OliveGlobal::open_action_search() {
    ActionSearch as(olive::MainWindow);
    as.exec();
}
