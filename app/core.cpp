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
#include <QCommandLineParser>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QHBoxLayout>

#include "dialog/sequence/sequence.h"
#include "panel/panelmanager.h"
#include "panel/project/project.h"
#include "project/item/footage/footage.h"
#include "project/item/sequence/sequence.h"
#include "render/colorservice.h"
#include "task/import/import.h"
#include "task/taskmanager.h"
#include "ui/style/style.h"
#include "undo/undostack.h"
#include "widget/menu/menushared.h"
#include "widget/taskview/taskviewitem.h"

Core olive::core;

Core::Core() :
  main_window_(nullptr),
  tool_(olive::tool::kPointer),
  snapping_(true)
{
}

void Core::Start()
{
  //
  // Parse command line arguments
  //

  QCoreApplication* app = QCoreApplication::instance();

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();

  // Project from command line option
  // FIXME: What's the correct way to make a visually "optional" positional argument, or is manually adding square
  // brackets like this correct?
  parser.addPositionalArgument("[project]", tr("Project to open on startup"));

  // Create fullscreen option
  QCommandLineOption fullscreen_option({"f", "fullscreen"}, tr("Start in full screen mode"));
  parser.addOption(fullscreen_option);

  // Parse options
  parser.process(*app);

  QStringList args = parser.positionalArguments();

  // Detect project to load on startup
  if (!args.isEmpty()) {
    startup_project_ = args.first();
  }

  // Declare custom types for Qt signal/slot syste
  DeclareTypesForQt();


  //
  // Start GUI (FIXME CLI mode)
  //

  StartGUI(parser.isSet(fullscreen_option));

  // Create new project on startup
  // FIXME: Load project from startup_project_ instead if not empty
  AddOpenProject(std::make_shared<Project>());
}

void Core::Stop()
{
  delete main_window_;
}

olive::MainWindow *Core::main_window()
{
  return main_window_;
}

void Core::ImportFiles(const QStringList &urls, ProjectViewModel* model, Folder* parent)
{
  if (urls.isEmpty()) {
    QMessageBox::critical(main_window_, tr("Import error"), tr("Nothing to import"));
    return;
  }

  olive::task_manager.AddTask(std::make_shared<ImportTask>(model, parent, urls));
}

const olive::tool::Tool &Core::tool()
{
  return tool_;
}

const bool &Core::snapping()
{
  return snapping_;
}

void Core::StartModalTask(Task *t)
{
  QDialog dialog(main_window_);

  QHBoxLayout* layout = new QHBoxLayout(&dialog);
  layout->setMargin(0);

  TaskViewItem* task_view = new TaskViewItem(&dialog);
  task_view->SetTask(t);
  layout->addWidget(task_view);

  connect(t, SIGNAL(Finished()), &dialog, SLOT(accept()));

  // FIXME: Risk of task finishing before dialog execs?

  if (t->Start()) {
    dialog.exec();
  }
}

void Core::SetTool(const olive::tool::Tool &tool)
{
  tool_ = tool;

  emit ToolChanged(tool_);
}

void Core::SetSnapping(const bool &b)
{
  snapping_ = b;

  emit SnappingChanged(snapping_);
}

void Core::DialogImportShow()
{
  // Open dialog for user to select files
  QStringList files = QFileDialog::getOpenFileNames(main_window_,
                                                    tr("Import footage..."));

  // Check if the user actually selected files to import
  if (!files.isEmpty()) {

    // Locate the most recently focused Project panel (assume that's the panel the user wants to import into)
    ProjectPanel* active_project_panel = olive::panel_focus_manager->MostRecentlyFocused<ProjectPanel>();
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

void Core::CreateNewFolder()
{
  // Locate the most recently focused Project panel (assume that's the panel the user wants to import into)
  ProjectPanel* active_project_panel = olive::panel_focus_manager->MostRecentlyFocused<ProjectPanel>();
  Project* active_project;

  if (active_project_panel == nullptr // Check that we found a Project panel
      || (active_project = active_project_panel->project()) == nullptr) { // and that we could find an active Project
    QMessageBox::critical(main_window_, tr("Failed to create new folder"), tr("Failed to find active Project panel"));
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

  olive::undo_stack.push(aic);

  // Trigger an automatic rename so users can enter the folder name
  active_project_panel->Edit(new_folder.get());
}

// FIXME: Test code
#include "node/blend/alphaover/alphaover.h"
#include "node/output/timeline/timeline.h"
#include "node/output/track/track.h"
#include "node/output/viewer/viewer.h"
#include "node/processor/renderer/renderer.h"
#include "panel/panelmanager.h"
#include "panel/node/node.h"
#include "panel/viewer/viewer.h"
// End test code

void Core::CreateNewSequence()
{
  // Locate the most recently focused Project panel (assume that's the panel the user wants to import into)
  ProjectPanel* active_project_panel = olive::panel_focus_manager->MostRecentlyFocused<ProjectPanel>();
  Project* active_project;

  if (active_project_panel == nullptr // Check that we found a Project panel
      || (active_project = active_project_panel->project()) == nullptr) { // and that we could find an active Project
    QMessageBox::critical(main_window_, tr("Failed to create new sequence"), tr("Failed to find active Project panel"));
    return;
  }

  // Get the selected folder in this panel
  Folder* folder = active_project_panel->GetSelectedFolder();

  // Create new sequence
  SequencePtr new_sequence = std::make_shared<Sequence>();

  // Set all defaults for the sequence
  new_sequence->SetDefaultParameters();

  // Get default name for this sequence (in the format "Sequence N", the first that doesn't exist)
  int sequence_number = 1;
  QString sequence_name;
  do {
    sequence_name = tr("Sequence %1").arg(sequence_number);
    sequence_number++;
  } while (active_project->root()->ChildExistsWithName(sequence_name));
  new_sequence->set_name(sequence_name);

  SequenceDialog sd(new_sequence.get(), SequenceDialog::kNew, main_window_);

  // Make sure SequenceDialog doesn't make an undo command for editing the sequence, since we make an undo command for
  // adding it later on
  sd.SetUndoable(false);

  if (sd.exec() == QDialog::Accepted) {
    // Create an undoable command
    ProjectViewModel::AddItemCommand* aic = new ProjectViewModel::AddItemCommand(active_project_panel->model(),
                                                                                 folder,
                                                                                 new_sequence);

    TimelineOutput* tb = new TimelineOutput();
    tb->SetTimebase(new_sequence->video_time_base());
    new_sequence->AddNode(tb);

    RendererProcessor* rp = new RendererProcessor();

    // Set renderer's parameters based on sequence's parameters
    rp->SetParameters(new_sequence->video_width(),
                      new_sequence->video_height(),
                      olive::PIX_FMT_RGBA16F, // FIXME: Make this configurable
                      olive::RenderMode::kOffline,
                      2);

    // Set the "cache name" only here to aid the cache ID's uniqueness
    rp->SetCacheName(new_sequence->name());
    rp->SetTimebase(new_sequence->video_time_base());

    new_sequence->AddNode(rp);

    ViewerOutput* vo = new ViewerOutput();
    vo->SetTimebase(new_sequence->video_time_base());
    new_sequence->AddNode(vo);

    TrackOutput* to = new TrackOutput();
    new_sequence->AddNode(to);

    // Connect track to renderer
    NodeParam::ConnectEdge(to->texture_output(), rp->texture_input());

    // Connect renderer to viewer
    NodeParam::ConnectEdge(rp->texture_output(), vo->texture_input());

    // Connect track to timeline
    NodeParam::ConnectEdge(to->track_output(), tb->track_input());

    // FIXME: Test code
    AlphaOverBlend* blend = new AlphaOverBlend();
    new_sequence->AddNode(blend);
    // End test code

    vo->AttachViewer(olive::panel_focus_manager->MostRecentlyFocused<ViewerPanel>());
    tb->AttachTimeline(olive::panel_focus_manager->MostRecentlyFocused<TimelinePanel>());
    olive::panel_focus_manager->MostRecentlyFocused<NodePanel>()->SetGraph(new_sequence.get());

    olive::undo_stack.push(aic);
  }
}

void Core::AddOpenProject(ProjectPtr p)
{
  open_projects_.append(p);

  emit ProjectOpened(p.get());
}

void Core::DeclareTypesForQt()
{
  qRegisterMetaType<Task::Status>("Task::Status");
  qRegisterMetaType<NodeDependency>();
}

void Core::StartGUI(bool full_screen)
{
  // Set UI style
  olive::style::AppSetDefault();

  // Set up shared menus
  olive::menu_shared.Initialize();

  // Since we're starting GUI mode, create a PanelFocusManager (auto-deletes with QObject)
  olive::panel_focus_manager = new PanelManager(this);

  // Connect the PanelFocusManager to the application's focus change signal
  connect(qApp,
          SIGNAL(focusChanged(QWidget*, QWidget*)),
          olive::panel_focus_manager,
          SLOT(FocusChanged(QWidget*, QWidget*)));

  // Create main window and open it
  main_window_ = new olive::MainWindow();
  if (full_screen) {
    main_window_->showFullScreen();
  } else {
    main_window_->showMaximized();
  }

  // When a new project is opened, update the mainwindow
  connect(this, SIGNAL(ProjectOpened(Project*)), main_window_, SLOT(ProjectOpen(Project*)));

  // Initialize color service
  ColorService::Init();
}

Project *Core::GetActiveProject()
{
  // Locate the most recently focused Project panel (assume that's the panel the user wants to import into)
  ProjectPanel* active_project_panel = olive::panel_focus_manager->MostRecentlyFocused<ProjectPanel>();

  // If we couldn't find one, return nullptr
  if (active_project_panel == nullptr) {
    return nullptr;
  }

  // Otherwise, return the project panel's project (which may be nullptr but in most cases shouldn't be)
  return active_project_panel->project();
}
