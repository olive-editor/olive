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

#include <QFileInfoList>
#include <QList>
#include <QTimer>

#include "common/rational.h"
#include "common/timecodefunctions.h"
#include "project/item/footage/footage.h"
#include "project/item/sequence/sequence.h"
#include "project/project.h"
#include "project/projectviewmodel.h"
#include "task/task.h"
#include "tool/tool.h"
#include "undo/undostack.h"

OLIVE_NAMESPACE_ENTER

class MainWindow;

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
   * @brief Core object accessible from anywhere in the code
   *
   * Use this to access Core functions.
   */
  static Core* instance();

  /**
   * @brief Start Olive Core
   *
   * Main application launcher. Parses command line arguments and constructs main window (if entering a GUI mode).
   */
  bool Start();

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
  MainWindow* main_window();

  /**
   * @brief Retrieve UndoStack object
   */
  UndoStack* undo_stack();

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
  const Tool::Item& tool() const;

  /**
   * @brief Get the currently selected object that the add tool should make (if the add tool is active)
   */
  const Tool::AddableObject& selected_addable_object() const;

  /**
   * @brief Get current snapping value
   */
  const bool& snapping() const;

  /**
   * @brief Returns a list of the most recently opened/saved projects
   */
  const QStringList& GetRecentProjects() const;

  /**
   * @brief Convenience function to retrieve a Project's shared pointer
   */
  ProjectPtr GetSharedPtrFromProject(Project* project) const;

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
  ProjectPtr GetActiveProject() const;
  ProjectViewModel* GetActiveProjectModel() const;
  Folder* GetSelectedFolderInActiveProject() const;

  /**
   * @brief Gets current timecode display mode
   */
  Timecode::Display GetTimecodeDisplay() const;

  /**
   * @brief Sets current timecode display mode
   */
  void SetTimecodeDisplay(Timecode::Display d);

  /**
   * @brief Set how frequently an autorecovery should be saved (if the project has changed, see SetProjectModified())
   */
  void SetAutorecoveryInterval(int minutes);

  static void CopyStringToClipboard(const QString& s);

  static QString PasteStringFromClipboard();

  /**
   * @brief Return a list of supported frame rates in rational form
   *
   * These rationals can be flipped to create a timebase in this frame rate.
   */
  static QList<rational> SupportedFrameRates();

  /**
   * @brief Return a list of supported sample rates in integer form
   */
  static QList<int> SupportedSampleRates();
  /**
   * @brief Return a list of supported channel layouts as or'd flags
   */
  static QList<uint64_t> SupportedChannelLayouts();

  /**
   * @brief Convert rational frame rate (i.e. flipped timebase) to a user-friendly string
   */
  static QString FrameRateToString(const rational& frame_rate);

  /**
   * @brief Convert integer sample rate to a user-friendly string
   */
  static QString SampleRateToString(const int &sample_rate);

  /**
   * @brief Convert channel layout to a user-friendly string
   */
  static QString ChannelLayoutToString(const uint64_t &layout);

  /**
   * @brief Recursively count files in a file/directory list
   */
  static int CountFilesInFileList(const QFileInfoList &filenames);

  static QVariant GetPreferenceForRenderMode(RenderMode::Mode mode, const QString& preference);
  static void SetPreferenceForRenderMode(RenderMode::Mode mode, const QString& preference, const QVariant& value);

  /**
   * @brief Create a new sequence named appropriately for the active project
   */
  SequencePtr CreateNewSequenceForProject(Project *project) const;

  /**
   * @brief Opens a project from the recently opened list
   */
  void OpenProjectFromRecentList(int index);

  enum CloseProjectBehavior {
    kCloseProjectOnlyOne,
    kCloseProjectAsk,
    kCloseProjectSave,
    kCloseProjectDontSave
  };

  /**
   * @brief Closes a project
   */
  bool CloseProject(ProjectPtr p, bool auto_open_new, CloseProjectBehavior& confirm_behavior);
  bool CloseProject(ProjectPtr p, bool auto_open_new);

  /**
   * @brief Closes all open projects
   */
  bool CloseAllProjects(bool auto_open_new);

public slots:
  /**
   * @brief Starts an open file dialog to load a project from file
   */
  void OpenProject();

  /**
   * @brief Save the currently active project
   *
   * If the project hasn't been saved before, this will be equivalent to calling SaveActiveProjectAs().
   */
  bool SaveActiveProject();

  /**
   * @brief Save the currently active project with a new filename
   */
  bool SaveActiveProjectAs();

  /**
   * @brief Save all currently open projects
   */
  bool SaveAllProjects();

  /**
   * @brief Closes the active project
   *
   * If no other projects are open, a new one is created automatically.
   */
  bool CloseActiveProject();

  /**
   * @brief Closes all projects except the active project
   */
  bool CloseAllExceptActiveProject();

  /**
   * @brief Closes all open projects
   *
   * Equivalent to `CloseAllProjects(true)`, but useful for the signal/slot system where you may not be able to specify
   * parameters.
   */
  bool CloseAllProjects();

  /**
   * @brief Set the current application-wide tool
   *
   * @param tool
   */
  void SetTool(const Tool::Item& tool);

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
   * @brief Show Export dialog
   */
  void DialogExportShow();

  /**
   * @brief Create a new folder in the currently active project
   */
  void CreateNewFolder();

  /**
   * @brief Create a new sequence in the currently active project
   */
  void CreateNewSequence();

  /**
   * @brief Set the currently selected object that the add tool should make
   */
  void SetSelectedAddableObject(const Tool::AddableObject& obj);

  /**
   * @brief Clears the list of recently opened/saved projects
   */
  void ClearOpenRecentList();

  /**
   * @brief Creates a new empty project and opens it
   */
  void CreateNewProject();

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
   * @brief Signal emitted when a project is closed
   */
  void ProjectClosed(Project* p);

  /**
   * @brief Signal emitted when the tool is changed from somewhere
   */
  void ToolChanged(const Tool::Item& tool);

  /**
   * @brief Signal emitted when the snapping setting is changed
   */
  void SnappingChanged(const bool& b);

  /**
   * @brief Signal emitted when the default timecode display mode changed
   */
  void TimecodeDisplayChanged(Timecode::Display d);

private:
  /**
   * @brief Get the file filter than can be used with QFileDialog to open and save compatible projects
   */
  static QString GetProjectFilter();

  /**
   * @brief Returns the filename where the recently opened/saved projects should be stored
   */
  static QString GetRecentProjectsFilePath();

  /**
   * @brief Saves a specific project
   */
  bool SaveProject(ProjectPtr p);

  /**
   * @brief Performs a "save as" on a specific project
   */
  bool SaveProjectAs(ProjectPtr p);

  /**
   * @brief Adds a filename to the top of the recently opened projects list (or moves it if it already exists)
   */
  void PushRecentlyOpenedProject(const QString &s);

  /**
   * @brief Internal project open
   */
  void OpenProjectInternal(const QString& filename);

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
   * @brief Internal function for saving a project to a file
   */
  void SaveProjectInternal(ProjectPtr project);

  /**
   * @brief Internal main window object
   */
  MainWindow* main_window_;

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
  Tool::Item tool_;

  /**
   * @brief Currently active addable object
   */
  Tool::AddableObject addable_object_;

  /**
   * @brief Current snapping setting
   */
  bool snapping_;

  /**
   * @brief Internal timer for saving autorecovery files
   */
  QTimer autorecovery_timer_;

  /**
   * @brief Application-wide undo stack instance
   */
  UndoStack undo_stack_;

  /**
   * @brief List of most recently opened/saved projects
   */
  QStringList recent_projects_;

  /**
   * @brief Internal variable for whether the GUI is active
   */
  bool gui_active_;

  /**
   * @brief Static singleton core instance
   */
  static Core instance_;

private slots:
  void SaveAutorecovery();

  void ProjectSaveSucceeded(OLIVE_NAMESPACE::ProjectPtr p);

  /**
   * @brief Adds a project to the "open projects" list
   */
  void AddOpenProject(OLIVE_NAMESPACE::ProjectPtr p);

  void ImportTaskComplete(QUndoCommand* command);

  bool ConfirmImageSequence(const QString &filename);

  void ProjectWasModified(bool e);

};

OLIVE_NAMESPACE_EXIT

#endif // CORE_H
