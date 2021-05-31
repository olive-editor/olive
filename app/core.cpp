/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "core.h"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QStyleFactory>
#ifdef Q_OS_WINDOWS
#include <QtPlatformHeaders/QWindowsWindowFunctions>
#endif

#include "audio/audiomanager.h"
#include "cli/clitask/clitaskdialog.h"
#include "codec/conformmanager.h"
#include "common/filefunctions.h"
#include "common/xmlutils.h"
#include "config/config.h"
#include "dialog/about/about.h"
#include "dialog/autorecovery/autorecoverydialog.h"
#include "dialog/export/export.h"
#include "dialog/footagerelink/footagerelinkdialog.h"
#include "dialog/sequence/sequence.h"
#include "dialog/task/task.h"
#include "dialog/preferences/preferences.h"
#include "node/color/colormanager/colormanager.h"
#include "node/factory.h"
#include "panel/panelmanager.h"
#include "panel/project/project.h"
#include "panel/viewer/viewer.h"
#include "render/diskmanager.h"
#include "render/framemanager.h"
#include "render/rendermanager.h"
#ifdef USE_OTIO
#include "task/project/loadotio/loadotio.h"
#include "task/project/saveotio/saveotio.h"
#endif
#include "task/project/import/import.h"
#include "task/project/import/importerrordialog.h"
#include "task/project/load/load.h"
#include "task/project/save/save.h"
#include "task/taskmanager.h"
#include "ui/style/style.h"
#include "undo/undostack.h"
#include "widget/menu/menushared.h"
#include "widget/taskview/taskviewitem.h"
#include "widget/viewer/viewer.h"
#include "widget/nodeparamview/nodeparamviewundo.h"
#include "window/mainwindow/mainstatusbar.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

Core* Core::instance_ = nullptr;
const uint Core::kProjectVersion = 210122;

Core::Core(const CoreParams& params) :
  main_window_(nullptr),
  tool_(Tool::kPointer),
  addable_object_(Tool::kAddableEmpty),
  snapping_(true),
  core_params_(params)
{
  // Store reference to this object, making the assumption that Core will only ever be made in
  // main(). This will obviously break if not.
  instance_ = this;

  translator_ = new QTranslator(this);
}

Core *Core::instance()
{
  return instance_;
}

void Core::DeclareTypesForQt()
{
  qRegisterMetaType<rational>();
  qRegisterMetaType<NodeValue>();
  qRegisterMetaType<NodeValueTable>();
  qRegisterMetaType<NodeValueDatabase>();
  qRegisterMetaType<FramePtr>();
  qRegisterMetaType<SampleBufferPtr>();
  qRegisterMetaType<AudioParams>();
  qRegisterMetaType<NodeKeyframe::Type>();
  qRegisterMetaType<Decoder::RetrieveState>();
  qRegisterMetaType<olive::TimeRange>();
  qRegisterMetaType<Color>();
  qRegisterMetaType<olive::AudioVisualWaveform>();
  qRegisterMetaType<olive::SampleJob>();
  qRegisterMetaType<olive::ShaderJob>();
  qRegisterMetaType<olive::GenerateJob>();
  qRegisterMetaType<olive::VideoParams>();
  qRegisterMetaType<olive::VideoParams::Interlacing>();
  qRegisterMetaType<olive::MainWindowLayoutInfo>();
  qRegisterMetaType<olive::RenderTicketPtr>();
}

void Core::Start()
{
  // Load application config
  Config::Load();

  // Set locale based on either startup arg, config, or auto-detect
  SetStartupLocale();

  // Declare custom types for Qt signal/slot system
  DeclareTypesForQt();

  // Set up node factory/library
  NodeFactory::Initialize();

  // Set up color manager's default config
  ColorManager::SetUpDefaultConfig();

  // Initialize task manager
  TaskManager::CreateInstance();

  // Initialize RenderManager
  RenderManager::CreateInstance();

  // Initialize FrameManager
  FrameManager::CreateInstance();

  // Initialize ConformManager
  ConformManager::CreateInstance();

  //
  // Start application
  //

  qInfo() << "Using Qt version:" << qVersion();

  switch (core_params_.run_mode()) {
  case CoreParams::kRunNormal:
    // Start GUI
    StartGUI(core_params_.fullscreen());

    // If we have a startup
    QMetaObject::invokeMethod(this, "OpenStartupProject", Qt::QueuedConnection);
    break;
  case CoreParams::kHeadlessExport:
    qInfo() << "Headless export is not fully implemented yet";
    break;
  case CoreParams::kHeadlessPreCache:
    qInfo() << "Headless pre-cache is not fully implemented yet";
    break;
  }
}

void Core::Stop()
{
  // Assume all projects have closed gracefully and no auto-recovery is necessary
  autorecovered_projects_.clear();
  SaveUnrecoveredList();

  // Save Config
  Config::Save();

  // Save recently opened projects
  {
    QFile recent_projects_file(GetRecentProjectsFilePath());
    if (recent_projects_file.open(QFile::WriteOnly | QFile::Text)) {
      QTextStream ts(&recent_projects_file);

      ts << recent_projects_.join('\n');

      recent_projects_file.close();
    }
  }

  ConformManager::DestroyInstance();

  FrameManager::DestroyInstance();

  RenderManager::DestroyInstance();

  MenuShared::DestroyInstance();

  TaskManager::DestroyInstance();

  PanelManager::DestroyInstance();

  AudioManager::DestroyInstance();

  DiskManager::DestroyInstance();

  NodeFactory::Destroy();

  delete main_window_;
  main_window_ = nullptr;
}

MainWindow *Core::main_window()
{
  return main_window_;
}

UndoStack *Core::undo_stack()
{
  return &undo_stack_;
}

void Core::ImportFiles(const QStringList &urls, ProjectViewModel* model, Folder* parent)
{
  if (urls.isEmpty()) {
    QMessageBox::critical(main_window_, tr("Import error"), tr("Nothing to import"));
    return;
  }

  ProjectImportTask* pim = new ProjectImportTask(model, parent, urls);

  if (!pim->GetFileCount()) {
    // No files to import
    delete pim;
    return;
  }

  TaskDialog* task_dialog = new TaskDialog(pim, tr("Importing..."), main_window());

  connect(task_dialog, &TaskDialog::TaskSucceeded, this, &Core::ImportTaskComplete);

  task_dialog->open();
}

const Tool::Item &Core::tool() const
{
  return tool_;
}

const Tool::AddableObject &Core::GetSelectedAddableObject() const
{
  return addable_object_;
}

const QString &Core::GetSelectedTransition() const
{
  return selected_transition_;
}

void Core::SetSelectedAddableObject(const Tool::AddableObject &obj)
{
  addable_object_ = obj;
}

void Core::SetSelectedTransitionObject(const QString &obj)
{
  selected_transition_ = obj;
}

void Core::ClearOpenRecentList()
{
  recent_projects_.clear();
  emit OpenRecentListChanged();
}

void Core::CreateNewProject()
{
  // If we already have an empty/new project, switch to it
  foreach (Project* already_open, open_projects_) {
    if (already_open->is_new()) {
      AddOpenProject(already_open);
      return;
    }
  }

  AddOpenProject(new Project());
}

const bool &Core::snapping() const
{
  return snapping_;
}

const QStringList &Core::GetRecentProjects() const
{
  return recent_projects_;
}

void Core::SetTool(const Tool::Item &tool)
{
  tool_ = tool;

  emit ToolChanged(tool_);
}

void Core::SetSnapping(const bool &b)
{
  snapping_ = b;

  emit SnappingChanged(snapping_);
}

void Core::DialogAboutShow()
{
  AboutDialog a(main_window_);
  a.exec();
}

void Core::DialogImportShow()
{
  // Open dialog for user to select files
  QStringList files = QFileDialog::getOpenFileNames(main_window_,
                                                    tr("Import footage..."));

  // Check if the user actually selected files to import
  if (!files.isEmpty()) {

    // Locate the most recently focused Project panel (assume that's the panel the user wants to import into)
    ProjectPanel* active_project_panel = PanelManager::instance()->MostRecentlyFocused<ProjectPanel>();
    Project* active_project;

    if (active_project_panel == nullptr // Check that we found a Project panel
        || (active_project = active_project_panel->project()) == nullptr) { // and that we could find an active Project
      QMessageBox::critical(main_window_, tr("Failed to import footage"), tr("Failed to find active Project panel"));
      return;
    }

    // Get the selected folder in this panel
    Folder* folder = active_project_panel->GetSelectedFolder();

    ImportFiles(files, active_project_panel->model(), folder);
  }
}

void Core::DialogPreferencesShow()
{
  PreferencesDialog pd(main_window_, main_window_->menuBar());
  pd.exec();
}

void Core::DialogExportShow()
{
  ViewerOutput* viewer = GetSequenceToExport();

  if (viewer) {
    ExportDialog* ed = new ExportDialog(viewer, main_window_);
    connect(ed, &ExportDialog::finished, ed, &ExportDialog::deleteLater);
    ed->open();
  }
}

void Core::CreateNewFolder()
{
  // Locate the most recently focused Project panel (assume that's the panel the user wants to import into)
  ProjectPanel* active_project_panel = PanelManager::instance()->MostRecentlyFocused<ProjectPanel>();
  Project* active_project;

  if (active_project_panel == nullptr // Check that we found a Project panel
      || (active_project = active_project_panel->project()) == nullptr) { // and that we could find an active Project
    QMessageBox::critical(main_window_, tr("Failed to create new folder"), tr("Failed to find active project"));
    return;
  }

  // Get the selected folder in this panel
  Folder* folder = active_project_panel->GetSelectedFolder();

  // Create new folder
  Folder* new_folder = new Folder();

  // Set a default name
  new_folder->SetLabel(tr("New Folder"));

  // Create an undoable command
  MultiUndoCommand* command = new MultiUndoCommand();

  command->add_child(new NodeAddCommand(active_project, new_folder));
  command->add_child(new FolderAddChild(folder, new_folder));

  Core::instance()->undo_stack()->push(command);

  // Trigger an automatic rename so users can enter the folder name
  active_project_panel->Edit(new_folder);
}

void Core::CreateNewSequence()
{
  Project* active_project = GetActiveProject();

  if (!active_project) {
    QMessageBox::critical(main_window_, tr("Failed to create new sequence"), tr("Failed to find active project"));
    return;
  }

  // Create new sequence
  Sequence* new_sequence = CreateNewSequenceForProject(active_project);

  SequenceDialog sd(new_sequence, SequenceDialog::kNew, main_window_);

  // Make sure SequenceDialog doesn't make an undo command for editing the sequence, since we make an undo command for
  // adding it later on
  sd.SetUndoable(false);

  if (sd.exec() == QDialog::Accepted) {

    // Create an undoable command
    MultiUndoCommand* command = new MultiUndoCommand();

    command->add_child(new NodeAddCommand(active_project, new_sequence));
    command->add_child(new FolderAddChild(GetSelectedFolderInActiveProject(), new_sequence));

    // Create and connect default nodes to new sequence
    new_sequence->add_default_nodes(command);

    Core::instance()->undo_stack()->push(command);

    Core::instance()->main_window()->OpenSequence(new_sequence);

  } else {

    // If the dialog was accepted, ownership goes to the AddItemCommand. But if we get here, just delete
    delete new_sequence;

  }
}

void Core::AddOpenProject(Project* p)
{
  // Ensure project is not open at the moment
  foreach (Project* already_open, open_projects_) {
    if (already_open == p) {
      // Signal UI to switch to this project
      emit ProjectOpened(p);
      return;
    }
  }

  // If we currently have an empty project, close it first
  if (!open_projects_.isEmpty() && open_projects_.last()->is_new()) {
    CloseProject(open_projects_.last(), false);
  }

  connect(p, &Project::ModifiedChanged, this, &Core::ProjectWasModified);
  open_projects_.append(p);

  PushRecentlyOpenedProject(p->filename());

  emit ProjectOpened(p);
}

bool Core::AddOpenProjectFromTask(Task *task)
{
  ProjectLoadBaseTask* load_task = static_cast<ProjectLoadBaseTask*>(task);

  Project* project = load_task->GetLoadedProject();
  MainWindowLayoutInfo layout = load_task->GetLoadedLayout();

  if (ValidateFootageInLoadedProject(project, load_task->GetFilenameProjectWasSavedAs())) {
    AddOpenProject(project);
    main_window_->LoadLayout(layout);

    return true;
  } else {
    delete project;

    return false;
  }
}

void Core::ImportTaskComplete(Task* task)
{
  ProjectImportTask* import_task = static_cast<ProjectImportTask*>(task);

  MultiUndoCommand *command = import_task->GetCommand();

  if (import_task->HasInvalidFiles()) {
    ProjectImportErrorDialog d(import_task->GetInvalidFiles(), main_window_);
    d.exec();
  }

  undo_stack_.pushIfHasChildren(command);
}

bool Core::ConfirmImageSequence(const QString& filename)
{
  QMessageBox mb(main_window_);

  mb.setIcon(QMessageBox::Question);
  mb.setWindowTitle(tr("Possible image sequence detected"));
  mb.setText(tr("The file '%1' looks like it might be part of an image "
                "sequence. Would you like to import it as such?").arg(filename));

  mb.addButton(QMessageBox::Yes);
  mb.addButton(QMessageBox::No);

  return (mb.exec() == QMessageBox::Yes);
}

void Core::ProjectWasModified(bool e)
{
  //Project* p = static_cast<Project*>(sender());

  if (e) {
    // If this project is modified, we know for sure the window should show a "modified" flag (the * in the titlebar)
    main_window_->setWindowModified(true);
  } else {
    // If we just set this project to "not modified", see if all projects are not modified in which case we can hide
    // the modified flag
    foreach (Project* open, open_projects_) {
      if (open->is_modified()) {
        main_window_->setWindowModified(true);
        return;
      }
    }

    main_window_->setWindowModified(false);
  }
}

bool Core::StartHeadlessExport()
{
  const QString& startup_project = core_params_.startup_project();

  if (startup_project.isEmpty()) {
    qCritical().noquote() << tr("You must specify a project file to export");
    return false;
  }

  if (!QFileInfo::exists(startup_project)) {
    qCritical().noquote() << tr("Specified project does not exist");
    return false;
  }

  // Start a load task and try running it
  ProjectLoadTask plm(startup_project);
  CLITaskDialog task_dialog(&plm);

  /*
  if (task_dialog.Run()) {
    std::unique_ptr<Project> p = std::unique_ptr<Project>(plm.GetLoadedProject());
    QVector<Item*> items = p->get_items_of_type(Item::kSequence);

    // Check if this project contains sequences
    if (items.isEmpty()) {
      qCritical().noquote() << tr("Project contains no sequences, nothing to export");
      return false;
    }

    Sequence* sequence = nullptr;

    // Check if this project contains multiple sequences
    if (items.size() > 1) {
      qInfo().noquote() << tr("This project has multiple sequences. Which do you wish to export?");
      for (int i=0;i<items.size();i++) {
        std::cout << "[" << i << "] " << items.at(i)->GetLabel().toStdString();
      }

      QTextStream stream(stdin);
      QString sequence_read;
      int sequence_index = -1;
      QString quit_code = QStringLiteral("q");
      std::string prompt = tr("Enter number (or %1 to cancel): ").arg(quit_code).toStdString();
      forever {
        std::cout << prompt;

        stream.readLineInto(&sequence_read);

        if (!QString::compare(sequence_read, quit_code, Qt::CaseInsensitive)) {
          return false;
        }

        bool ok;
        sequence_index = sequence_read.toInt(&ok);

        if (ok && sequence_index >= 0 && sequence_index < items.size()) {
          break;
        } else {
          qCritical().noquote() << tr("Invalid sequence number");
        }
      }

      sequence = static_cast<Sequence*>(items.at(sequence_index));
    } else {
      sequence = static_cast<Sequence*>(items.first());
    }

    ExportParams params;
    ExportTask export_task(sequence->viewer_output(), p->color_manager(), params);
    CLITaskDialog export_dialog(&export_task);
    if (export_dialog.Run()) {
      qInfo().noquote() << tr("Export succeeded");
      return true;
    } else {
      qInfo().noquote() << tr("Export failed: %1").arg(export_task.GetError());
      return false;
    }
  } else {
    qCritical().noquote() << tr("Project failed to load: %1").arg(plm.GetError());
    return false;
  }
  */



  return false;
}

void Core::OpenStartupProject()
{
  const QString& startup_project = core_params_.startup_project();
  bool startup_project_exists = !startup_project.isEmpty() && QFileInfo::exists(startup_project);

  // Load startup project
  if (!startup_project_exists && !startup_project.isEmpty()) {
    QMessageBox::warning(main_window_,
                         tr("Failed to open startup file"),
                         tr("The project \"%1\" doesn't exist. "
                            "A new project will be started instead.").arg(startup_project),
                         QMessageBox::Ok);
  }

  if (startup_project_exists) {
    // If a startup project was set and exists, open it now
    OpenProjectInternal(startup_project);
  } else {
    // If no load project is set, create a new one on open
    CreateNewProject();
  }
}

void Core::AddRecoveryProjectFromTask(Task *task)
{
  if (AddOpenProjectFromTask(task)) {
    ProjectLoadBaseTask* load_task = static_cast<ProjectLoadBaseTask*>(task);

    Project* project = load_task->GetLoadedProject();

    // Clearing the filename will force the user to re-save it somewhere else
    project->set_filename(QString());

    // Forcing a UUID regeneration will prevent it from saving auto-recoveries in the same place
    // the original project did
    project->RegenerateUuid();

    // Setting modified will ensure that the program doesn't close and lose the project without
    // prompting the user first
    project->set_modified(true);
  }
}

void Core::StartGUI(bool full_screen)
{
  // Set UI style
  StyleManager::Init();

  // Set up shared menus
  MenuShared::CreateInstance();

  // Since we're starting GUI mode, create a PanelFocusManager (auto-deletes with QObject)
  PanelManager::CreateInstance();

  // Initialize audio service
  AudioManager::CreateInstance();

  // Initialize disk service
  DiskManager::CreateInstance();

  // Connect the PanelFocusManager to the application's focus change signal
  connect(qApp,
          &QApplication::focusChanged,
          PanelManager::instance(),
          &PanelManager::FocusChanged);

  // Create main window and open it
  main_window_ = new MainWindow();

  if (full_screen) {
    main_window_->showFullScreen();
  } else {
    main_window_->showMaximized();
  }

#ifdef Q_OS_WINDOWS
  // Workaround for Qt bug where menus don't appear in full screen mode
  // See: https://doc.qt.io/qt-5/windows-issues.html
  QWindowsWindowFunctions::setHasBorderInFullScreen(main_window_->windowHandle(), true);
#endif

  // When a new project is opened, update the mainwindow
  connect(this, &Core::ProjectOpened, main_window_, &MainWindow::ProjectOpen, Qt::QueuedConnection);
  connect(this, &Core::ProjectClosed, main_window_, &MainWindow::ProjectClose);

  // Start autorecovery timer using the config value as its interval
  SetAutorecoveryInterval(Config::Current()["AutorecoveryInterval"].toInt());
  connect(&autorecovery_timer_, &QTimer::timeout, this, &Core::SaveAutorecovery);
  autorecovery_timer_.start();

  // Load recently opened projects list
  {
    QFile recent_projects_file(GetRecentProjectsFilePath());
    if (recent_projects_file.open(QFile::ReadOnly | QFile::Text)) {
      QTextStream ts(&recent_projects_file);

      QString s;
      while (!(s = ts.readLine()).isEmpty()) {
        recent_projects_.append(s);
      }

      recent_projects_file.close();
    }

    emit OpenRecentListChanged();
  }
}

void Core::SaveProjectInternal(Project* project, const QString& override_filename)
{
  // Create save manager
  Task* psm;

  if (project->filename().endsWith(QStringLiteral(".otio"), Qt::CaseInsensitive)) {
#ifdef USE_OTIO
    psm = new SaveOTIOTask(project);
#else
    QMessageBox::critical(main_window_,
                          tr("Missing OpenTimelineIO Libraries"),
                          tr("This build was compiled without OpenTimelineIO and therefore "
                             "cannot open OpenTimelineIO files."));
    return;
#endif
  } else {
    psm = new ProjectSaveTask(project);

    if (!override_filename.isEmpty()) {
      // Set override filename if provided
      static_cast<ProjectSaveTask*>(psm)->SetOverrideFilename(override_filename);
    }
  }

  // We don't use a TaskDialog here because a model save dialog is annoying, particularly when
  // saving auto-recoveries that the user can't anticipate. Doing this in the main thread will
  // cause a brief (but often unnoticeable) pause in the GUI, which, while not ideal, is not that
  // different from what already happened (modal dialog preventing use of the GUI) and in many ways
  // less annoying (doesn't disrupt any current actions or pull focus from elsewhere).
  //
  // Ideally we could do this in a background thread and show progress in the status bar like
  // Microsoft Word, but that would be far more complex. If it becomes necessary in the future,
  // we will look into an approach like that.
  if (psm->Start()) {
    if (override_filename.isEmpty()) {
      ProjectSaveSucceeded(psm);
    }
  }

  psm->deleteLater();
}

ViewerOutput* Core::GetSequenceToExport()
{
  // First try the most recently focused time based window
  TimeBasedPanel* time_panel = PanelManager::instance()->MostRecentlyFocused<TimeBasedPanel>();

  // If that fails try defaulting to the first timeline (i.e. if a project has just been loaded).
  if (!time_panel->GetConnectedViewer()) {
    // Safe to assume there will always be one timeline.
    time_panel = PanelManager::instance()->GetPanelsOfType<TimelinePanel>().first();
  }

  if (time_panel && time_panel->GetConnectedViewer()) {
    if (time_panel->GetConnectedViewer()->GetLength() == 0) {
      QMessageBox::critical(main_window_,
                            tr("Error"),
                            tr("This Sequence is empty. There is nothing to export."),
                            QMessageBox::Ok);
    } else {
      return time_panel->GetConnectedViewer();
    }
  } else {
    QMessageBox::critical(main_window_,
                          tr("Error"),
                          tr("No valid sequence detected.\n\nMake sure a sequence is loaded and it has a connected Viewer node."),
                          QMessageBox::Ok);
  }

  return nullptr;
}

QString Core::GetAutoRecoveryIndexFilename()
{
  return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).filePath(QStringLiteral("unrecovered"));
}

void Core::SaveUnrecoveredList()
{
  QFile autorecovery_index(GetAutoRecoveryIndexFilename());

  if (autorecovered_projects_.isEmpty()) {
    // Recovery list is empty, delete file if exists
    if (autorecovery_index.exists()) {
      autorecovery_index.remove();
    }
  } else if (autorecovery_index.open(QFile::WriteOnly)) {
    // Overwrite recovery list with current list
    QTextStream ts(&autorecovery_index);

    bool first = true;
    foreach (const QUuid& uuid, autorecovered_projects_) {
      if (first) {
        first = false;
      } else {
        ts << QStringLiteral("\n");
      }
      ts << uuid.toString();
    }

    autorecovery_index.close();
  } else {
    qWarning() << "Failed to save unrecovered list";
  }
}

void Core::SaveAutorecovery()
{
  if (Config::Current()[QStringLiteral("AutorecoveryEnabled")].toBool()) {
    foreach (Project* p, open_projects_) {
      if (!p->has_autorecovery_been_saved()) {
        QDir project_autorecovery_dir(QDir(FileFunctions::GetAutoRecoveryRoot()).filePath(p->GetUuid().toString()));
        if (project_autorecovery_dir.mkpath(QStringLiteral("."))) {
          QString this_autorecovery_path = project_autorecovery_dir.filePath(QStringLiteral("%1.ove").arg(QString::number(QDateTime::currentSecsSinceEpoch())));

          SaveProjectInternal(p, this_autorecovery_path);

          p->set_autorecovery_saved(true);

          // Keep track of projects that where the "newest" save is the recovery project
          if (!autorecovered_projects_.contains(p->GetUuid())) {
            autorecovered_projects_.append(p->GetUuid());
          }

          qDebug() << "Saved auto-recovery to:" << this_autorecovery_path;

          // Write human-readable real name so it's not just a UUID
          {
            QFile realname_file(project_autorecovery_dir.filePath(QStringLiteral("realname.txt")));
            realname_file.open(QFile::WriteOnly);
            realname_file.write(p->pretty_filename().toUtf8());
            realname_file.close();
          }

          int64_t max_recoveries_per_file = Config::Current()[QStringLiteral("AutorecoveryMaximum")].toLongLong();

          // Since we write an extra file, increment total allowed files by 1
          max_recoveries_per_file++;

          // Delete old entries
          QStringList recovery_files = project_autorecovery_dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
          while (recovery_files.size() > max_recoveries_per_file) {
            bool deleted = false;
            for (int i=0; i<recovery_files.size(); i++) {
              const QString& f = recovery_files.at(i);

              if (f.endsWith(QStringLiteral(".ove"), Qt::CaseInsensitive)) {
                QString delete_full_path = project_autorecovery_dir.filePath(f);
                qDebug() << "Deleted old recovery:" << delete_full_path;
                QFile::remove(delete_full_path);
                recovery_files.removeAt(i);
                deleted = true;
                break;
              }
            }

            if (!deleted) {
              // For some reason none of the files were deletable. Break so we don't end up in
              // an infinite loop.
              break;
            }
          }
        } else {
          QMessageBox::critical(main_window_, tr("Auto-Recovery Error"),
                                tr("Failed to save auto-recovery to \"%1\". "
                                   "Olive may not have permission to this directory.")
                                .arg(project_autorecovery_dir.absolutePath()));
        }
      }
    }

    // Save index
    SaveUnrecoveredList();
  }
}

void Core::ProjectSaveSucceeded(Task* task)
{
  Project* p = static_cast<ProjectSaveTask*>(task)->GetProject();

  PushRecentlyOpenedProject(p->filename());

  p->set_modified(false);

  autorecovered_projects_.removeOne(p->GetUuid());
  SaveUnrecoveredList();
}

Project* Core::GetActiveProject() const
{
  ProjectPanel* active_project_panel = PanelManager::instance()->MostRecentlyFocused<ProjectPanel>();

  if (active_project_panel && active_project_panel->project()) {
    return active_project_panel->project();
  }

  return nullptr;
}

ProjectViewModel *Core::GetActiveProjectModel() const
{
  ProjectPanel* active_project_panel = PanelManager::instance()->MostRecentlyFocused<ProjectPanel>();

  if (active_project_panel) {
    return active_project_panel->model();
  } else {
    return nullptr;
  }
}

Folder *Core::GetSelectedFolderInActiveProject() const
{
  ProjectPanel* active_project_panel = PanelManager::instance()->MostRecentlyFocused<ProjectPanel>();

  if (active_project_panel) {
    return active_project_panel->GetSelectedFolder();
  } else {
    return nullptr;
  }
}

Timecode::Display Core::GetTimecodeDisplay() const
{
  return static_cast<Timecode::Display>(Config::Current()["TimecodeDisplay"].toInt());
}

void Core::SetTimecodeDisplay(Timecode::Display d)
{
  Config::Current()["TimecodeDisplay"] = d;

  emit TimecodeDisplayChanged(d);
}

void Core::SetAutorecoveryInterval(int minutes)
{
  // Convert minutes to milliseconds
  autorecovery_timer_.setInterval(minutes * 60000);
}

void Core::CopyStringToClipboard(const QString &s)
{
  QGuiApplication::clipboard()->setText(s);
}

QString Core::PasteStringFromClipboard()
{
  return QGuiApplication::clipboard()->text();
}

bool Core::SaveActiveProject()
{
  Project* active_project = GetActiveProject();

  if (active_project) {
    return SaveProject(active_project);
  }

  return false;
}

bool Core::SaveActiveProjectAs()
{
  Project* active_project = GetActiveProject();

  if (active_project) {
    return SaveProjectAs(active_project);
  }

  return false;
}

bool Core::SaveAllProjects()
{
  foreach (Project* p, open_projects_) {
    if (!SaveProject(p)) {
      return false;
    }
  }

  return true;
}

bool Core::CloseActiveProject()
{
  return CloseProject(GetActiveProject(), true);
}

bool Core::CloseAllExceptActiveProject()
{
  Project* active_proj = GetActiveProject();
  QList<Project*> copy = open_projects_;

  foreach (Project* p, copy) {
    if (p != active_proj) {
      if (!CloseProject(p, true)) {
        return false;
      }
    }
  }

  return true;
}

QString Core::GetProjectFilter(bool include_any_filter)
{
  QString filters;

#ifdef USE_OTIO
  if (include_any_filter) {
    filters.append(QStringLiteral("All Supported Projects (*.ove *.otio);;"));
  }
#else
  Q_UNUSED(include_any_filter)
#endif

  // Append standard filter
  filters.append(QStringLiteral("%1 (*.ove)").arg(tr("Olive Project")));

#ifdef USE_OTIO
  filters.append(QStringLiteral(";;%2 (*.otio)").arg(tr("OpenTimelineIO")));
#endif

  return filters;
}

QString Core::GetRecentProjectsFilePath()
{
  return QDir(FileFunctions::GetConfigurationLocation()).filePath(QStringLiteral("recent"));
}

void Core::SetStartupLocale()
{
  // Set language
  if (!core_params_.startup_language().isEmpty()) {
    if (translator_->load(core_params_.startup_language()) && QApplication::installTranslator(translator_)) {
      return;
    } else {
      qWarning() << "Failed to load translation file. Falling back to defaults.";
    }
  }

  QString use_locale = Config::Current()[QStringLiteral("Language")].toString();

  if (use_locale.isEmpty()) {
    // No configured locale, auto-detect the system's locale
    use_locale = QLocale::system().name();
  }

  if (!SetLanguage(use_locale)) {
    qWarning() << "Trying to use locale" << use_locale << "but couldn't find a translation for it";
  }
}

bool Core::SaveProject(Project* p)
{
  if (p->filename().isEmpty()) {
    return SaveProjectAs(p);
  } else {
    SaveProjectInternal(p);

    return true;
  }
}

void Core::ShowStatusBarMessage(const QString &s, int timeout)
{
  main_window_->statusBar()->showMessage(s, timeout);
}

void Core::ClearStatusBarMessage()
{
  main_window_->statusBar()->clearMessage();
}

void Core::OpenRecoveryProject(const QString &filename)
{
  OpenProjectInternal(filename, true);
}

void Core::OpenNodeInViewer(ViewerOutput *viewer)
{
  main_window_->OpenNodeInViewer(viewer);
}

void Core::CheckForAutoRecoveries()
{
  QFile autorecovery_index(GetAutoRecoveryIndexFilename());
  if (autorecovery_index.exists()) {
    // Uh-oh, we have auto-recoveries to prompt
    if (autorecovery_index.open(QFile::ReadOnly)) {
      QStringList recovery_filenames = QString::fromUtf8(autorecovery_index.readAll()).split('\n');

      AutoRecoveryDialog ard(tr("The following projects had unsaved changes when Olive "
                                "forcefully quit. Would you like to load them?"),
                             recovery_filenames, true, main_window_);
      ard.exec();

      autorecovery_index.close();

      // Delete recovery index since we don't need it anymore
      QFile::remove(GetAutoRecoveryIndexFilename());
    } else {
      QMessageBox::critical(main_window_, tr("Auto-Recovery Error"),
                            tr("Found auto-recoveries but failed to load the auto-recovery index. "
                               "Auto-recover projects will have to be opened manually.\n\n"
                               "Your recoverable projects are still available at: %1").arg(FileFunctions::GetAutoRecoveryRoot()));
    }
  }
}

void Core::BrowseAutoRecoveries()
{
  // List all auto-recovery entries
  AutoRecoveryDialog ard(tr("The following project versions have been auto-saved:"),
                         QDir(FileFunctions::GetAutoRecoveryRoot()).entryList(QDir::Dirs | QDir::NoDotAndDotDot),
                         false, main_window_);
  ard.exec();
}

bool Core::SaveProjectAs(Project* p)
{
  QFileDialog fd(main_window_, tr("Save Project As"));

  fd.setAcceptMode(QFileDialog::AcceptSave);
  fd.setNameFilter(GetProjectFilter(false));

  if (fd.exec() == QDialog::Accepted) {
    QString fn = fd.selectedFiles().first();

    // Somewhat hacky method of extracting the extension from the name filter
    const QString& name_filter = fd.selectedNameFilter();
    int ext_index = name_filter.indexOf(QStringLiteral("(*.")) + 3;
    QString extension = name_filter.mid(ext_index, name_filter.size() - ext_index - 1);

    fn = FileFunctions::EnsureFilenameExtension(fn, extension);

    p->set_filename(fn);

    SaveProjectInternal(p);

    return true;
  }

  return false;
}

void Core::PushRecentlyOpenedProject(const QString& s)
{
  if (s.isEmpty()) {
    return;
  }

  int existing_index = recent_projects_.indexOf(s);

  if (existing_index >= 0) {
    recent_projects_.move(existing_index, 0);
  } else {
    recent_projects_.prepend(s);
  }

  emit OpenRecentListChanged();
}

void Core::OpenProjectInternal(const QString &filename, bool recovery_project)
{
  // See if this project is open already
  foreach (Project* p, open_projects_) {
    if (p->filename() == filename) {
      // This project is already open
      AddOpenProject(p);
      return;
    }
  }

  Task* load_task;

  if (filename.endsWith(QStringLiteral(".otio"), Qt::CaseInsensitive)) {
    // Load OpenTimelineIO project
#ifdef USE_OTIO
    load_task = new LoadOTIOTask(filename);
#else
    QMessageBox::critical(main_window_,
                          tr("Missing OpenTimelineIO Libraries"),
                          tr("This build was compiled without OpenTimelineIO and therefore "
                             "cannot open OpenTimelineIO files."));
    return;
#endif
  } else {
    // Fallback to regular OVE project
    load_task = new ProjectLoadTask(filename);
  }

  TaskDialog* task_dialog = new TaskDialog(load_task, tr("Load Project"), main_window());

  if (recovery_project) {
    connect(task_dialog, &TaskDialog::TaskSucceeded, this, &Core::AddRecoveryProjectFromTask);
  } else {
    connect(task_dialog, &TaskDialog::TaskSucceeded, this, &Core::AddOpenProjectFromTask);
  }

  task_dialog->open();
}

int Core::CountFilesInFileList(const QFileInfoList &filenames)
{
  int file_count = 0;

  foreach (const QFileInfo& f, filenames) {
    // For some reason QDir::NoDotAndDotDot	doesn't work with entryInfoList, so we have to check manually
    if (f.fileName() == "." || f.fileName() == "..") {
      continue;
    } else if (f.isDir()) {
      QFileInfoList info_list = QDir(f.absoluteFilePath()).entryInfoList();

      file_count += CountFilesInFileList(info_list);
    } else {
      file_count++;
    }
  }

  return file_count;
}

QString GetRenderModePreferencePrefix(RenderMode::Mode mode, const QString &preference) {
  QString key;

  key.append((mode == RenderMode::kOffline) ? QStringLiteral("Offline") : QStringLiteral("Online"));
  key.append(preference);

  return key;
}

QVariant Core::GetPreferenceForRenderMode(RenderMode::Mode mode, const QString &preference)
{
  return Config::Current()[GetRenderModePreferencePrefix(mode, preference)];
}

void Core::SetPreferenceForRenderMode(RenderMode::Mode mode, const QString &preference, const QVariant &value)
{
  Config::Current()[GetRenderModePreferencePrefix(mode, preference)] = value;
}

void Core::LabelNodes(const QVector<Node *> &nodes)
{
  if (nodes.isEmpty()) {
    return;
  }

  bool ok;

  QString start_label = nodes.first()->GetLabel();

  for (int i=1; i<nodes.size(); i++) {
    if (nodes.at(i)->GetLabel() != start_label) {
      // Not all the nodes share the same name, so we'll start with a blank one
      start_label.clear();
      break;
    }
  }

  QString s = QInputDialog::getText(main_window_,
                                    tr("Label Node"),
                                    tr("Set node label"),
                                    QLineEdit::Normal,
                                    start_label,
                                    &ok);

  if (ok) {
    NodeRenameCommand* rename_command = new NodeRenameCommand();

    foreach (Node* n, nodes) {
      rename_command->AddNode(n, s);
    }

    undo_stack_.push(rename_command);
  }
}

Sequence *Core::CreateNewSequenceForProject(Project* project) const
{
  Sequence* new_sequence = new Sequence();

  // Get default name for this sequence (in the format "Sequence N", the first that doesn't exist)
  int sequence_number = 1;
  QString sequence_name;
  do {
    sequence_name = tr("Sequence %1").arg(sequence_number);
    sequence_number++;
  } while (project->root()->ChildExistsWithName(sequence_name));
  new_sequence->SetLabel(sequence_name);

  return new_sequence;
}

void Core::OpenProjectFromRecentList(int index)
{
  const QString& open_fn = recent_projects_.at(index);

  if (QFileInfo::exists(open_fn)) {
    OpenProjectInternal(open_fn);
  } else if (QMessageBox::information(main_window(),
                                      tr("Cannot open recent project"),
                                      tr("The project \"%1\" doesn't exist. Would you like to remove this file from the recent list?").arg(open_fn),
                                      QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
    recent_projects_.removeAt(index);

    emit OpenRecentListChanged();
  }
}

bool Core::CloseProject(Project *p, bool auto_open_new)
{
  CloseProjectBehavior b = kCloseProjectOnlyOne;
  return CloseProject(p, auto_open_new, b);
}

bool Core::CloseProject(Project* p, bool auto_open_new, CloseProjectBehavior &confirm_behavior)
{
  for (int i=0;i<open_projects_.size();i++) {
    if (open_projects_.at(i) == p) {

      if (p->is_modified() && confirm_behavior != kCloseProjectDontSave) {

        bool save_this_project;

        if (confirm_behavior == kCloseProjectAsk || confirm_behavior == kCloseProjectOnlyOne) {
          QMessageBox mb(main_window_);

          mb.setWindowModality(Qt::WindowModal);
          mb.setIcon(QMessageBox::Question);
          mb.setWindowTitle(tr("Unsaved Changes"));
          mb.setText(tr("The project '%1' has unsaved changes. Would you like to save them?")
                     .arg(p->name()));

          QPushButton* yes_btn = mb.addButton(tr("Save"), QMessageBox::YesRole);

          QPushButton* yes_to_all_btn;
          if (confirm_behavior == kCloseProjectOnlyOne) {
            yes_to_all_btn = nullptr;
          } else {
            yes_to_all_btn = mb.addButton(tr("Save All"), QMessageBox::YesRole);
          }

          mb.addButton(tr("Don't Save"), QMessageBox::NoRole);

          QPushButton* no_to_all_btn;
          if (confirm_behavior == kCloseProjectOnlyOne) {
            no_to_all_btn = nullptr;
          } else {
            no_to_all_btn = mb.addButton(tr("Don't Save All"), QMessageBox::NoRole);
          }

          QPushButton* cancel_btn = mb.addButton(QMessageBox::Cancel);

          mb.exec();

          if (mb.clickedButton() == cancel_btn) {
            // Stop closing projects if the user clicked cancel
            return false;
          } else if (mb.clickedButton() == yes_to_all_btn) {
            // Set flag that other CloseProject commands are going to use
            confirm_behavior = kCloseProjectSave;
          } else if (mb.clickedButton() == no_to_all_btn) {
            // Set flag that other CloseProject commands are going to use
            confirm_behavior = kCloseProjectDontSave;
          }

          save_this_project = (mb.clickedButton() == yes_btn || mb.clickedButton() == yes_to_all_btn);

        } else {
          // We must be saving this project
          save_this_project = true;
        }

        if (save_this_project && !SaveProject(p)) {
          // The save failed, stop closing projects
          return false;
        }

      }

      // For safety, the undo stack is cleared so no commands try to affect a freed project
      undo_stack_.clear();

      disconnect(p, &Project::ModifiedChanged, this, &Core::ProjectWasModified);
      emit ProjectClosed(p);
      open_projects_.removeAt(i);
      delete p;
      break;
    }
  }

  // Ensure a project is always active
  if (auto_open_new && open_projects_.isEmpty()) {
    CreateNewProject();
  }

  return true;
}

bool Core::CloseAllProjects(bool auto_open_new)
{
  QList<Project*> copy = open_projects_;

  // See how many projects are modified so we can set "behavior" correctly
  // (i.e. whether to show "Yes/No To All" buttons or not)
  int modified_count = 0;
  foreach (Project* p, copy) {
    if (p->is_modified()) {
      modified_count++;
    }
  }

  CloseProjectBehavior behavior;

  if (modified_count > 1) {
    behavior = kCloseProjectAsk;
  } else {
    behavior = kCloseProjectOnlyOne;
  }

  foreach (Project* p, copy) {
    // If this is the only remaining project and the user hasn't chose "yes/no to all", hide those buttons
    if (modified_count == 1 && behavior == kCloseProjectAsk) {
      behavior = kCloseProjectOnlyOne;
    }

    if (!CloseProject(p, auto_open_new, behavior)) {
      return false;
    }

    modified_count--;
  }

  return true;
}

void Core::CacheActiveSequence(bool in_out_only)
{
  TimeBasedPanel* p = PanelManager::instance()->MostRecentlyFocused<TimeBasedPanel>();

  if (p && p->GetConnectedViewer()) {
    // Hacky but works for now

    // Find Viewer attached to this TimeBasedPanel
    QList<ViewerPanel*> all_viewers = PanelManager::instance()->GetPanelsOfType<ViewerPanel>();

    ViewerPanel* found_panel = nullptr;

    foreach (ViewerPanel* viewer, all_viewers) {
      if (viewer->GetConnectedViewer() == p->GetConnectedViewer()) {
        found_panel = viewer;
        break;
      }
    }

    if (found_panel) {
      if (in_out_only) {
        found_panel->CacheSequenceInOut();
      } else {
        found_panel->CacheEntireSequence();
      }
    } else {
      QMessageBox::critical(main_window_,
                            tr("Failed to cache sequence"),
                            tr("No active viewer found with this sequence."),
                            QMessageBox::Ok);
    }
  }
}

bool Core::ValidateFootageInLoadedProject(Project* project, const QString& project_saved_url)
{
  QVector<Footage*> project_footage = project->root()->ListChildrenOfType<Footage>();
  QVector<Footage*> footage_we_couldnt_validate;

  foreach (Footage* footage, project_footage) {
    if (!QFileInfo::exists(footage->filename()) && !project_saved_url.isEmpty()) {
      // If the footage doesn't exist, it might have moved with the project
      const QString& project_current_url = project->filename();

      if (project_current_url != project_saved_url) {
        // Project has definitely moved, try to resolve relative paths
        QDir saved_dir(QFileInfo(project_saved_url).dir());
        QDir true_dir(QFileInfo(project_current_url).dir());

        QString relative_filename = saved_dir.relativeFilePath(footage->filename());
        QString transformed_abs_filename = true_dir.filePath(relative_filename);

        if (QFileInfo::exists(transformed_abs_filename)) {
          // Use this file instead
          qInfo() << "Resolved" << footage->filename() << "relatively to" << transformed_abs_filename;
          footage->set_filename(transformed_abs_filename);
        }
      }
    }

    if (QFileInfo::exists(footage->filename())) {
      // Assume valid
      footage->SetValid();
    } else {
      footage_we_couldnt_validate.append(footage);
    }
  }

  if (!footage_we_couldnt_validate.isEmpty()) {
    FootageRelinkDialog frd(footage_we_couldnt_validate, main_window_);
    if (frd.exec() == QDialog::Rejected) {
      return false;
    }
  }

  return true;
}

bool Core::SetLanguage(const QString &locale)
{
  QApplication::removeTranslator(translator_);

  QString resource_path = QStringLiteral(":/ts/%1").arg(locale);
  if (translator_->load(resource_path) && QApplication::installTranslator(translator_)) {
    return true;
  }

  return false;
}

bool Core::CloseAllProjects()
{
  return CloseAllProjects(true);
}

void Core::OpenProject()
{
  QString file = QFileDialog::getOpenFileName(main_window_,
                                              tr("Open Project"),
                                              QString(),
                                              GetProjectFilter(true));

  if (!file.isEmpty()) {
    OpenProjectInternal(file);
  }
}

Core::CoreParams::CoreParams() :
  mode_(kRunNormal),
  run_fullscreen_(false)
{
}

}
