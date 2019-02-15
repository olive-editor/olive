#include "oliveglobal.h"

#include "mainwindow.h"

#include "panels/panels.h"

#include "io/path.h"

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
#include <QDebug>

QSharedPointer<OliveGlobal> Olive::Global;
QString Olive::ActiveProjectFilename;
QString Olive::AppName;

OliveGlobal::OliveGlobal() {
    // sets current app name
    QString version_id;
#ifdef GITHASH
    version_id = QString(" | %1").arg(GITHASH);
#endif
    Olive::AppName = QString("Olive (February 2019 | Alpha%1)").arg(version_id);

    // set the file filter used in all file dialogs pertaining to Olive project files.
    project_file_filter = tr("Olive Project %1").arg("(*.ove)");

    // set default value
    enable_load_project_on_init = false;
}

const QString &OliveGlobal::get_project_file_filter() {
    return project_file_filter;
}

void OliveGlobal::update_project_filename(const QString &s) {
    // set filename to s
    Olive::ActiveProjectFilename = s;

    // update main window title to reflect new project filename
    Olive::MainWindow->updateTitle();
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
    Olive::ActiveProjectFilename = s;
    enable_load_project_on_init = true;
}

QString OliveGlobal::get_recent_project_list_file() {
    return get_data_dir().filePath("recents");
}

void OliveGlobal::new_project() {
    if (can_close_project()) {
        // clear effects panel
        panel_effect_controls->clear_effects(true);

        // clear project contents (footage, sequences, etc.)
        panel_project->new_project();

        // clear undo stack
        Olive::UndoStack.clear();

        // empty current project filename
        update_project_filename("");

        // full update of all panels
        update_ui(false);
    }
}

void OliveGlobal::open_project() {
    QString fn = QFileDialog::getOpenFileName(Olive::MainWindow, tr("Open Project..."), "", project_file_filter);
    if (!fn.isEmpty() && can_close_project()) {
        open_project_worker(fn, false);
    }
}

void OliveGlobal::open_recent() {
    int index = static_cast<QAction*>(sender())->data().toInt();
    QString recent_url = recent_projects.at(index);
    if (!QFile::exists(recent_url)) {
        if (QMessageBox::question(
                        Olive::MainWindow,
                        tr("Missing recent project"),
                        tr("The project '%1' no longer exists. Would you like to remove it from the recent projects list?").arg(recent_url),
                        QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
            recent_projects.removeAt(index);
            panel_project->save_recent_projects();
        }
    } else if (Olive::Global.data()->can_close_project()) {
        open_project_worker(recent_url, false);
    }
}

bool OliveGlobal::save_project_as() {
    QString fn = QFileDialog::getSaveFileName(Olive::MainWindow, tr("Save Project As..."), "", project_file_filter);
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
    if (Olive::ActiveProjectFilename.isEmpty()) {
        return save_project_as();
    } else {
        panel_project->save_project(false);
        return true;
    }
}

bool OliveGlobal::can_close_project() {
    if (Olive::MainWindow->isWindowModified()) {
        QMessageBox* m = new QMessageBox(
                    QMessageBox::Question,
                    tr("Unsaved Project"),
                    tr("This project has changed since it was last saved. Would you like to save it before closing?"),
                    QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,
                    Olive::MainWindow
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
    if (Olive::ActiveSequence == nullptr) {
        QMessageBox::information(Olive::MainWindow,
                                 tr("No active sequence"),
                                 tr("Please open the sequence you wish to export."),
                                 QMessageBox::Ok);
    } else {
        ExportDialog e(Olive::MainWindow);
        e.exec();
    }
}

void OliveGlobal::finished_initialize() {
    // if a project was set as a command line argument, we load it here
    if (enable_load_project_on_init) {
        open_project_worker(Olive::ActiveProjectFilename, false);
        enable_load_project_on_init = false;
    } else {
        // if we are not loading a project on launch and are running a release build, open the demo notice dialog
#ifndef QT_DEBUG
        DemoNotice* d = new DemoNotice(Olive::MainWindow);
        connect(d, SIGNAL(finished(int)), d, SLOT(deleteLater()));
        d->open();
#endif
    }
}

void OliveGlobal::save_autorecovery_file() {
    if (Olive::MainWindow->isWindowModified()) {
        panel_project->save_project(true);
        qInfo() << "Auto-recovery project saved";
    }
}

void OliveGlobal::open_preferences() {
    panel_sequence_viewer->pause();
    panel_footage_viewer->pause();

    PreferencesDialog pd(Olive::MainWindow);
    pd.setup_kbd_shortcuts(Olive::MainWindow->menuBar());
    pd.exec();
}

void OliveGlobal::open_project_worker(const QString& fn, bool autorecovery) {
    update_project_filename(fn);
    panel_project->load_project(autorecovery);
    Olive::UndoStack.clear();
}

void OliveGlobal::undo() {
    // workaround to prevent crash (and also users should never need to do this)
    if (!panel_timeline->importing) {
        Olive::UndoStack.undo();
        update_ui(true);
    }
}

void OliveGlobal::redo() {
    // workaround to prevent crash (and also users should never need to do this)
    if (!panel_timeline->importing) {
        Olive::UndoStack.redo();
        update_ui(true);
    }
}

void OliveGlobal::paste() {
    if (Olive::ActiveSequence != nullptr) {
        panel_timeline->paste(false);
    }
}

void OliveGlobal::paste_insert() {
    if (Olive::ActiveSequence != nullptr) {
        panel_timeline->paste(true);
    }
}

void OliveGlobal::open_about_dialog() {
    AboutDialog a(Olive::MainWindow);
    a.exec();
}

void OliveGlobal::open_debug_log() {
    Olive::DebugDialog->show();
}

void OliveGlobal::open_speed_dialog() {
    if (Olive::ActiveSequence != nullptr) {
        SpeedDialog s(Olive::MainWindow);
        for (int i=0;i<Olive::ActiveSequence->clips.size();i++) {
            Clip* c = Olive::ActiveSequence->clips.at(i);
            if (c != nullptr && is_clip_selected(c, true)) {
                s.clips.append(c);
            }
        }
        if (s.clips.size() > 0) s.run();
    }
}

void OliveGlobal::clear_undo_stack() {
    Olive::UndoStack.clear();
}

void OliveGlobal::open_action_search() {
    ActionSearch as(Olive::MainWindow);
    as.exec();
}
