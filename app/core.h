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

#ifndef CORE_H
#define CORE_H

#include <QList>

#include "project/project.h"
#include "project/projectviewmodel.h"
#include "window/mainwindow/mainwindow.h"
#include "task/task.h"
#include "tool/tool.h"

/**
 * @brief The main central Olive application instance
 *
 * This runs both in GUI and CLI modes (and handles what to init based on that).
 * It also contains various global functions/variables for use throughout Olive.
 *
 * The "public slots" are usually user-triggered actions and can be connected to UI elements (e.g. creating a folder,
 * opening the import dialog, etc.)
 */
class Core : public QObject
{
  Q_OBJECT
public:
  /**
   * @brief Core Constructor
   *
   * Currently empty
   */
  Core();

  /**
   * @brief Start Olive Core
   *
   * Main application launcher. Parses command line arguments and constructs main window (if entering a GUI mode).
   */
  void Start();

  /**
   * @brief Stop Olive Core
   *
   * Ends all threads and frees all memory ready for the application to exit.
   */
  void Stop();

  /**
   * @brief Retrieve main window instance
   *
   * @return
   *
   * Pointer to the olive::MainWindow object, or nullptr if running in CLI mode.
   */
  olive::MainWindow* main_window();

  /**
   * @brief Import a list of files
   *
   * FIXME: I kind of hate this, it needs a model to update correctly. Is there a way that Items can signal enough to
   *        make passing references to the model unnecessary?
   *
   * @param urls
   */
  void ImportFiles(const QStringList& urls, ProjectViewModel *model, Folder *parent);

  /**
   * @brief Get the currently active tool
   */
  const olive::tool::Tool& tool();

  /**
   * @brief Get current snapping value
   */
  const bool& snapping();

  /**
   * @brief Starts a modal task
   *
   * This function does NOT take ownership of the Task.
   *
   * @param t
   */
  void StartModalTask(Task* t);

  /**
   * @brief Get the currently active project
   *
   * Uses the UI/Panel system to determine which Project was the last focused on and assumes this is the active Project
   * that the user wishes to work on.
   *
   * @return
   *
   * The active Project file, or nullptr if the heuristic couldn't find one.
   */
  Project* GetActiveProject();

public slots:
  /**
   * @brief Set the current application-wide tool
   *
   * @param tool
   */
  void SetTool(const olive::tool::Tool& tool);

  /**
   * @brief Set the current snapping setting
   */
  void SetSnapping(const bool& b);

  /**
   * @brief Show an About dialog
   */
  void DialogAboutShow();

  /**
   * @brief Open the import footage dialog and import the files selected (runs ImportFiles())
   */
  void DialogImportShow();

  /**
   * @brief Show Preferences dialog
   */
  void DialogPreferencesShow();

  /**
   * @brief Show Project Properties dialog
   */
  void DialogProjectPropertiesShow();

  /**
   * @brief Create a new folder in the currently active project
   */
  void CreateNewFolder();

  /**
   * @brief Createa a new sequence in the currently active project
   */
  void CreateNewSequence();

signals:
  /**
   * @brief Signal emitted when a project is opened
   *
   * Connects to main window so its UI can update based on the project
   *
   * @param p
   */
  void ProjectOpened(Project* p);

  /**
   * @brief Signal emitted when the tool is changed from somewhere
   */
  void ToolChanged(const olive::tool::Tool& tool);

  /**
   * @brief Signal emitted when the snapping setting is changed
   */
  void SnappingChanged(const bool& b);

private:
  /**
   * @brief Creates an empty project and adds it to the "open projects"
   */
  void AddOpenProject(ProjectPtr p);

  /**
   * @brief Declare custom types/classes for Qt's signal/slot system
   *
   * Qt's signal/slot system requires types to be declared. In the interest of doing this only at startup, we contain
   * them all in a function here.
   */
  void DeclareTypesForQt();

  /**
   * @brief Start GUI portion of Olive
   *
   * Starts services and objects required for the GUI of Olive. It's guaranteed that running without this function will
   * create an application instance that is completely valid minus the UI (e.g. for CLI modes).
   */
  void StartGUI(bool full_screen);

  /**
   * @brief Internal main window object
   */
  olive::MainWindow* main_window_;

  /**
   * @brief Internal startup project object
   *
   * If the user specifies a project file on the command line, the command line parser in Start() will write the
   * project URL here to be loaded once Olive has finished initializing.
   */
  QString startup_project_;

  /**
   * @brief List of currently open projects
   */
  QList<ProjectPtr> open_projects_;

  /**
   * @brief Currently active tool
   */
  olive::tool::Tool tool_;

  /**
   * @brief Current snapping toggle
   */
  bool snapping_;

};

namespace olive {
/**
 * @brief Core object accessible from anywhere in the code
 *
 * Use this to access Core functions.
 */
extern Core core;
}

#endif // CORE_H
