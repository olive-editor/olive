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

#include "decoder/probeserver.h"
#include "panel/panelfocusmanager.h"
#include "panel/project/project.h"
#include "project/item/footage/footage.h"
#include "ui/icons/icons.h"
#include "ui/style/style.h"
#include "widget/menu/menushared.h"

Core olive::core;

Core::Core() :
  main_window_(nullptr)
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



  //
  // Start GUI (TODO CLI mode)
  //

  // Load all icons
  olive::icon::LoadAll();

  // Set UI style
  olive::style::AppSetDefault();

  // Set up shared menus
  olive::menu_shared.Initialize();

  // Since we're starting GUI mode, create a PanelFocusManager (auto-deletes with QObject)
  olive::panel_focus_manager = new PanelFocusManager(this);

  // Connect the PanelFocusManager to the application's focus change signal
  connect(app,
          SIGNAL(focusChanged(QWidget*, QWidget*)),
          olive::panel_focus_manager,
          SLOT(FocusChanged(QWidget*, QWidget*)));

  // Create main window and open it
  main_window_ = new olive::MainWindow();
  if (parser.isSet(fullscreen_option)) {
    main_window_->showFullScreen();
  } else {
    main_window_->showMaximized();
  }

  // When a new project is opened, update the mainwindow
  connect(this, SIGNAL(ProjectOpened(Project*)), main_window_, SLOT(ProjectOpen(Project*)));

  // Create new project on startup
  // TODO: Load project from startup_project_ instead if not empty
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

void Core::ImportFiles(const QStringList &urls)
{
  QString error_capt = tr("Import error");

  if (urls.isEmpty()) {
    QMessageBox::critical(main_window_, error_capt, tr("Nothing to import"));
    return;
  }

  ProjectPanel* active_project_panel = olive::panel_focus_manager->MostRecentlyFocused<ProjectPanel>();
  Project* active_project;

  if (active_project_panel == nullptr // Check that we found a Project panel
      || (active_project = active_project_panel->project()) == nullptr) { // and that we could find an active Project

    QMessageBox::critical(main_window_, error_capt, tr("Failed to find active Project panel"));

  } else {

    for (int i=0;i<urls.size();i++) {
      const QString& url = urls.at(i);

      Footage* f = new Footage();

      QFileInfo file_info(url);

      f->set_filename(url);
      f->set_name(file_info.fileName());
      //file_info.lastModified();

      olive::ProbeMedia(f);

      active_project->root()->add_child(f);

    }

    // FIXME: Dumb way of resetting the model to update for new information
    active_project_panel->set_project(active_project);
  }
}

void Core::SetTool(const olive::tool::Tool &tool)
{
  tool_ = tool;

  emit ToolChanged(tool_);
}

void Core::StartImportFootage()
{
  // Open dialog for user to select files
  QStringList files = QFileDialog::getOpenFileNames(main_window_,
                                                    tr("Import footage..."));

  // Check if the user actually selected files to import
  if (!files.isEmpty()) {
    ImportFiles(files);
  }
}

void Core::AddOpenProject(ProjectPtr p)
{
  open_projects_.append(p);
  emit ProjectOpened(p.get());
}
