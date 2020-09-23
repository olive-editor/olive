/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "audio/audiomanager.h"
#include "cli/clitask/clitaskdialog.h"
#include "common/filefunctions.h"
#include "common/xmlutils.h"
#include "config/config.h"
#include "dialog/about/about.h"
#include "dialog/export/export.h"
#include "dialog/sequence/sequence.h"
#include "dialog/task/task.h"
#include "dialog/preferences/preferences.h"
#include "dialog/projectproperties/projectproperties.h"
#include "node/factory.h"
#include "panel/panelmanager.h"
#include "panel/project/project.h"
#include "panel/viewer/viewer.h"
#include "render/backend/opengl/opengltexturecache.h"
#include "render/colormanager.h"
#include "render/diskmanager.h"
#include "render/pixelformat.h"
#include "render/shaderinfo.h"
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
#include "window/mainwindow/mainwindow.h"

OLIVE_NAMESPACE_ENTER

Core* Core::instance_ = nullptr;

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
}

Core *Core::instance()
{
  return instance_;
}

void Core::DeclareTypesForQt()
{
  qRegisterMetaType<rational>();
  qRegisterMetaType<OpenGLTexturePtr>();
  qRegisterMetaType<OpenGLTextureCache::ReferencePtr>();
  qRegisterMetaType<NodeValue>();
  qRegisterMetaType<NodeValueTable>();
  qRegisterMetaType<NodeValueDatabase>();
  qRegisterMetaType<FramePtr>();
  qRegisterMetaType<SampleBufferPtr>();
  qRegisterMetaType<AudioParams>();
  qRegisterMetaType<NodeKeyframe::Type>();
  qRegisterMetaType<Decoder::RetrieveState>();
  qRegisterMetaType<OLIVE_NAMESPACE::TimeRange>();
  qRegisterMetaType<Color>();
  qRegisterMetaType<OLIVE_NAMESPACE::ProjectPtr>();
  qRegisterMetaType<OLIVE_NAMESPACE::AudioVisualWaveform>();
  qRegisterMetaType<OLIVE_NAMESPACE::SampleJob>();
  qRegisterMetaType<OLIVE_NAMESPACE::ShaderJob>();
  qRegisterMetaType<OLIVE_NAMESPACE::GenerateJob>();
  qRegisterMetaType<OLIVE_NAMESPACE::VideoParams>();
  qRegisterMetaType<OLIVE_NAMESPACE::VideoParams::Interlacing>();
  qRegisterMetaType<OLIVE_NAMESPACE::MainWindowLayoutInfo>();
  qRegisterMetaType<OLIVE_NAMESPACE::RenderTicketPtr>();
}

void Core::Start()
{
  // Load application config
  Config::Load();

  // Declare custom types for Qt signal/slot system
  DeclareTypesForQt();

  // Set up node factory/library
  NodeFactory::Initialize();

  // Set up color manager's default config
  ColorManager::SetUpDefaultConfig();

  // Initialize task manager
  TaskManager::CreateInstance();

  // Initialize OpenGL service
  OpenGLProxy::CreateInstance();

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
  // Save Config
  Config::Save();

  // Save recently opened projects
  {
    QFile recent_projects_file(GetRecentProjectsFilePath());
    if (recent_projects_file.open(QFile::WriteOnly | QFile::Text)) {
      QTextStream ts(&recent_projects_file);

      foreach (const QString& s, recent_projects_) {
        ts << s << "\n";
      }

      recent_projects_file.close();
    }
  }

  OpenGLProxy::DestroyInstance();

  MenuShared::DestroyInstance();

  TaskManager::DestroyInstance();

  PanelManager::DestroyInstance();

  AudioManager::DestroyInstance();

  DiskManager::DestroyInstance();

  PixelFormat::DestroyInstance();

  NodeFactory::Destroy();

  delete main_window_;
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
}

void Core::CreateNewProject()
{
  // If we already have an empty/new project, switch to it
  foreach (ProjectPtr already_open, open_projects_) {
    if (already_open->is_new()) {
      AddOpenProject(already_open);
      return;
    }
  }

  AddOpenProject(std::make_shared<Project>());
}

const bool &Core::snapping() const
{
  return snapping_;
}

const QStringList &Core::GetRecentProjects() const
{
  return recent_projects_;
}

ProjectPtr Core::GetSharedPtrFromProject(Project *project) const
{
  foreach (ProjectPtr p, open_projects_) {
    if (p.get() == project) {
      return p;
    }
  }

  return nullptr;
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

void Core::DialogProjectPropertiesShow()
{
  ProjectPtr proj = GetActiveProject();

  if (proj) {
    ProjectPropertiesDialog ppd(proj.get(), main_window_);
    ppd.exec();
  } else {
    QMessageBox::critical(main_window_,
                          tr("No Active Project"),
                          tr("No project is currently open to set the properties for"),
                          QMessageBox::Ok);
  }
}

void Core::DialogExportShow()
{
  SequenceViewerPanel* latest_sequence = PanelManager::instance()->MostRecentlyFocused<SequenceViewerPanel>();

  if (latest_sequence && latest_sequence->GetConnectedViewer()) {
    if (latest_sequence->GetConnectedViewer()->GetLength() == 0) {
      QMessageBox::critical(main_window_,
                            tr("Error"),
                            tr("This Sequence is empty. There is nothing to export."),
                            QMessageBox::Ok);
    } else {
      ExportDialog ed(latest_sequence->GetConnectedViewer(), main_window_);
      ed.exec();
    }
  } else {
    QMessageBox::critical(main_window_,
                          tr("Error"),
                          tr("No valid sequence detected.\nMake sure a sequence is loaded and it has a connected Viewer node."),
                          QMessageBox::Ok);
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
  ItemPtr new_folder = std::make_shared<Folder>();

  // Set a default name
  new_folder->set_name(tr("New Folder"));

  // Create an undoable command
  ProjectViewModel::AddItemCommand* aic = new ProjectViewModel::AddItemCommand(active_project_panel->model(),
                                                                               folder,
                                                                               new_folder);

  Core::instance()->undo_stack()->push(aic);

  // Trigger an automatic rename so users can enter the folder name
  active_project_panel->Edit(new_folder.get());
}

void Core::CreateNewSequence()
{
  ProjectPtr active_project = GetActiveProject();

  if (!active_project) {
    QMessageBox::critical(main_window_, tr("Failed to create new sequence"), tr("Failed to find active project"));
    return;
  }

  // Create new sequence
  SequencePtr new_sequence = CreateNewSequenceForProject(active_project.get());

  // Set all defaults for the sequence
  new_sequence->set_default_parameters();

  SequenceDialog sd(new_sequence.get(), SequenceDialog::kNew, main_window_);

  // Make sure SequenceDialog doesn't make an undo command for editing the sequence, since we make an undo command for
  // adding it later on
  sd.SetUndoable(false);

  if (sd.exec() == QDialog::Accepted) {
    // Create an undoable command
    ProjectViewModel::AddItemCommand* aic = new ProjectViewModel::AddItemCommand(GetActiveProjectModel(),
                                                                                 GetSelectedFolderInActiveProject(),
                                                                                 new_sequence);

    new_sequence->add_default_nodes();

    Core::instance()->undo_stack()->push(aic);

    Core::instance()->main_window()->OpenSequence(new_sequence.get());
  }
}

void Core::AddOpenProject(ProjectPtr p)
{
  // Ensure project is not open at the moment
  foreach (ProjectPtr already_open, open_projects_) {
    if (already_open == p) {
      // Signal UI to switch to this project
      emit ProjectOpened(p.get());
      return;
    }
  }

  // If we currently have an empty project, close it first
  if (!open_projects_.isEmpty() && open_projects_.last()->is_new()) {
    CloseProject(open_projects_.last(), false);
  }

  connect(p.get(), &Project::ModifiedChanged, this, &Core::ProjectWasModified);
  open_projects_.append(p);

  PushRecentlyOpenedProject(p->filename());

  emit ProjectOpened(p.get());
}

void Core::AddOpenProjectFromTask(Task *task)
{
  QList<ProjectPtr> projects = static_cast<ProjectLoadTask*>(task)->GetLoadedProjects();
  QList<MainWindowLayoutInfo> layouts = static_cast<ProjectLoadTask*>(task)->GetLoadedLayouts();

  for (int i=0; i<projects.size(); i++) {
    AddOpenProject(projects.at(i));
    main_window_->LoadLayout(layouts.at(i));
  }
}

void Core::ImportTaskComplete(Task* task)
{
  ProjectImportTask* import_task = static_cast<ProjectImportTask*>(task);

  QUndoCommand *command = import_task->GetCommand();

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
    foreach (ProjectPtr open, open_projects_) {
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

  if (task_dialog.Run()) {
    ProjectPtr p = plm.GetLoadedProjects().first();
    QList<ItemPtr> items = p->get_items_of_type(Item::kSequence);

    // Check if this project contains sequences
    if (items.isEmpty()) {
      qCritical().noquote() << tr("Project contains no sequences, nothing to export");
      return false;
    }

    SequencePtr sequence = nullptr;

    // Check if this project contains multiple sequences
    if (items.size() > 1) {
      qInfo().noquote() << tr("This project has multiple sequences. Which do you wish to export?");
      for (int i=0;i<items.size();i++) {
        std::cout << "[" << i << "] " << items.at(i)->name().toStdString();
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

      sequence = std::static_pointer_cast<Sequence>(items.at(sequence_index));
    } else {
      sequence = std::static_pointer_cast<Sequence>(items.first());
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

  // Initialize pixel service
  PixelFormat::CreateInstance();

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

  // When a new project is opened, update the mainwindow
  connect(this, &Core::ProjectOpened, main_window_, &MainWindow::ProjectOpen);
  connect(this, &Core::ProjectClosed, main_window_, &MainWindow::ProjectClose);

  // Start autorecovery timer using the config value as its interval
  SetAutorecoveryInterval(Config::Current()["AutorecoveryInterval"].toInt());
  autorecovery_timer_.start();

  // Load recently opened projects list
  {
    QFile recent_projects_file(GetRecentProjectsFilePath());
    if (recent_projects_file.open(QFile::ReadOnly | QFile::Text)) {
      QTextStream ts(&recent_projects_file);

      while (!ts.atEnd()) {
        recent_projects_.append(ts.readLine());
      }

      recent_projects_file.close();
    }
  }
}

void Core::SaveProjectInternal(ProjectPtr project)
{
  // Create save manager
  ProjectSaveTask* psm = new ProjectSaveTask(project);
  TaskDialog* task_dialog = new TaskDialog(psm, tr("Save Project"), main_window_);

  connect(task_dialog, &TaskDialog::TaskSucceeded, this, &Core::ProjectSaveSucceeded);

  task_dialog->open();
}

void Core::SaveAutorecovery()
{
  foreach (ProjectPtr p, open_projects_) {
    if (!p->has_autorecovery_been_saved()) {
      // FIXME: SAVE AN AUTORECOVERY PROJECT
      p->set_autorecovery_saved(true);
    }
  }
}

void Core::ProjectSaveSucceeded(Task* task)
{
  ProjectPtr p = static_cast<ProjectSaveTask*>(task)->GetProject();

  PushRecentlyOpenedProject(p->filename());

  p->set_modified(false);
}

ProjectPtr Core::GetActiveProject() const
{
  ProjectPanel* active_project_panel = PanelManager::instance()->MostRecentlyFocused<ProjectPanel>();

  if (active_project_panel && active_project_panel->project()) {
    return GetSharedPtrFromProject(active_project_panel->project());
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
  ProjectPtr active_project = GetActiveProject();

  if (active_project) {
    return SaveProject(active_project);
  }

  return false;
}

bool Core::SaveActiveProjectAs()
{
  ProjectPtr active_project = GetActiveProject();

  if (active_project) {
    return SaveProjectAs(active_project);
  }

  return false;
}

bool Core::SaveAllProjects()
{
  foreach (ProjectPtr p, open_projects_) {
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
  ProjectPtr active_proj = GetActiveProject();
  QList<ProjectPtr> copy = open_projects_;

  foreach (ProjectPtr p, copy) {
    if (p != active_proj) {
      if (!CloseProject(p, true)) {
        return false;
      }
    }
  }

  return true;
}

QString Core::GetProjectFilter()
{
  return QStringLiteral("%1 (*.ove)").arg(tr("Olive Project"));
}

QString Core::GetRecentProjectsFilePath()
{
  return QDir(FileFunctions::GetConfigurationLocation()).filePath(QStringLiteral("recent"));
}

bool Core::SaveProject(ProjectPtr p)
{
  if (p->filename().isEmpty()) {
    return SaveProjectAs(p);
  } else {
    SaveProjectInternal(p);

    return true;
  }
}

bool Core::SaveProjectAs(ProjectPtr p)
{
  QString fn = QFileDialog::getSaveFileName(main_window_,
                                            tr("Save Project As"),
                                            QString(),
                                            GetProjectFilter());

  if (!fn.isEmpty()) {
    QString extension(QStringLiteral(".ove"));
    if (!fn.endsWith(extension, Qt::CaseInsensitive)) {
      fn.append(extension);
    }

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
}

void Core::OpenProjectInternal(const QString &filename)
{
  // See if this project is open already
  foreach (ProjectPtr p, open_projects_) {
    if (p->filename() == filename) {
      // This project is already open
      AddOpenProject(p);
      return;
    }
  }

  ProjectLoadTask* plm = new ProjectLoadTask(filename);

  TaskDialog* task_dialog = new TaskDialog(plm, tr("Load Project"), main_window());

  connect(task_dialog, &TaskDialog::TaskSucceeded, this, &Core::AddOpenProjectFromTask);

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

void Core::LabelNodes(const QList<Node *> &nodes) const
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
    foreach (Node* n, nodes) {
      n->SetLabel(s);
    }
  }
}

SequencePtr Core::CreateNewSequenceForProject(Project* project) const
{
  SequencePtr new_sequence = std::make_shared<Sequence>();

  // Get default name for this sequence (in the format "Sequence N", the first that doesn't exist)
  int sequence_number = 1;
  QString sequence_name;
  do {
    sequence_name = tr("Sequence %1").arg(sequence_number);
    sequence_number++;
  } while (project->root()->ChildExistsWithName(sequence_name));
  new_sequence->set_name(sequence_name);

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
  }
}

bool Core::CloseProject(ProjectPtr p, bool auto_open_new)
{
  CloseProjectBehavior b = kCloseProjectOnlyOne;
  return CloseProject(p, auto_open_new, b);
}

bool Core::CloseProject(ProjectPtr p, bool auto_open_new, CloseProjectBehavior &confirm_behavior)
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

      disconnect(p.get(), &Project::ModifiedChanged, this, &Core::ProjectWasModified);
      emit ProjectClosed(p.get());
      open_projects_.removeAt(i);
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
  QList<ProjectPtr> copy = open_projects_;

  // See how many projects are modified so we can set "behavior" correctly
  // (i.e. whether to show "Yes/No To All" buttons or not)
  int modified_count = 0;
  foreach (ProjectPtr p, copy) {
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

  foreach (ProjectPtr p, copy) {
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

bool Core::CloseAllProjects()
{
  return CloseAllProjects(true);
}

void Core::OpenProject()
{
  QString file = QFileDialog::getOpenFileName(main_window_,
                                              tr("Open Project"),
                                              QString(),
                                              GetProjectFilter());

  if (!file.isEmpty()) {
    OpenProjectInternal(file);
  }
}

Core::CoreParams::CoreParams() :
  mode_(kRunNormal),
  run_fullscreen_(false)
{
}

OLIVE_NAMESPACE_EXIT
