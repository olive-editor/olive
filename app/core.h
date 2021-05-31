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

#ifndef CORE_H
#define CORE_H

#include <QFileInfoList>
#include <QList>
#include <QTimer>
#include <QTranslator>

#include "common/rational.h"
#include "common/timecodefunctions.h"
#include "node/project/footage/footage.h"
#include "node/project/project.h"
#include "node/project/projectviewmodel.h"
#include "node/project/sequence/sequence.h"
#include "task/task.h"
#include "tool/tool.h"
#include "undo/undostack.h"

namespace olive {

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
  class CoreParams
  {
  public:
    CoreParams();

    enum RunMode {
      kRunNormal,
      kHeadlessExport,
      kHeadlessPreCache
    };

    bool fullscreen() const
    {
      return run_fullscreen_;
    }

    void set_fullscreen(bool e)
    {
      run_fullscreen_ = e;
    }

    RunMode run_mode() const
    {
      return mode_;
    }

    void set_run_mode(RunMode m)
    {
      mode_ = m;
    }

    const QString startup_project() const
    {
      return startup_project_;
    }

    void set_startup_project(const QString& p)
    {
      startup_project_ = p;
    }

    const QString& startup_language() const
    {
      return startup_language_;
    }

    void set_startup_language(const QString& s)
    {
      startup_language_ = s;
    }

  private:
    RunMode mode_;

    QString startup_project_;

    QString startup_language_;

    bool run_fullscreen_;

  };

  /**
   * @brief Core Constructor
   *
   * Currently empty
   */
  Core(const CoreParams& params);

  /**
   * @brief Core object accessible from anywhere in the code
   *
   * Use this to access Core functions.
   */
  static Core* instance();

  const CoreParams& core_params() const
  {
    return core_params_;
  }

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
  const Tool::AddableObject& GetSelectedAddableObject() const;

  /**
   * @brief Get the currently selected node that the transition tool should make (if the transition tool is active)
   */
  const QString& GetSelectedTransition() const;

  /**
   * @brief Get current snapping value
   */
  const bool& snapping() const;

  /**
   * @brief Returns a list of the most recently opened/saved projects
   */
  const QStringList& GetRecentProjects() const;

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
  Project* GetActiveProject() const;
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
   * @brief Recursively count files in a file/directory list
   */
  static int CountFilesInFileList(const QFileInfoList &filenames);

  static QVariant GetPreferenceForRenderMode(RenderMode::Mode mode, const QString& preference);
  static void SetPreferenceForRenderMode(RenderMode::Mode mode, const QString& preference, const QVariant& value);

  /**
   * @brief Show a dialog to the user to rename a set of nodes
   */
  void LabelNodes(const QVector<Node *> &nodes);

  /**
   * @brief Create a new sequence named appropriately for the active project
   */
  Sequence* CreateNewSequenceForProject(Project *project) const;

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
  bool CloseProject(Project* p, bool auto_open_new, CloseProjectBehavior& confirm_behavior);
  bool CloseProject(Project* p, bool auto_open_new);

  /**
   * @brief Closes all open projects
   */
  bool CloseAllProjects(bool auto_open_new);

  /**
   * @brief Runs a modal cache task on the currently active sequence
   */
  void CacheActiveSequence(bool in_out_only);

  /**
   * @brief Check each footage object for whether it still exists or has changed
   */
  bool ValidateFootageInLoadedProject(Project* project, const QString &project_saved_url);

  /**
   * @brief Changes the current language
   */
  bool SetLanguage(const QString& locale);

  /**
   * @brief Saves a specific project
   */
  bool SaveProject(Project *p);

  /**
   * @brief Show message in main window's status bar
   *
   * Shorthand for Core::instance()->main_window()->statusBar()->showMessage();
   */
  void ShowStatusBarMessage(const QString& s, int timeout = 0);

  void ClearStatusBarMessage();

  void OpenRecoveryProject(const QString& filename);

  void OpenNodeInViewer(ViewerOutput* viewer);

  static const uint kProjectVersion;

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
   * @brief Set the currently selected object that the add tool should make
   */
  void SetSelectedTransitionObject(const QString& obj);

  /**
   * @brief Clears the list of recently opened/saved projects
   */
  void ClearOpenRecentList();

  /**
   * @brief Creates a new empty project and opens it
   */
  void CreateNewProject();

  void CheckForAutoRecoveries();

  void BrowseAutoRecoveries();

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

  /**
   * @brief Signal emitted when a change is made to the open recent list
   */
  void OpenRecentListChanged();

private:
  /**
   * @brief Get the file filter than can be used with QFileDialog to open and save compatible projects
   */
  static QString GetProjectFilter(bool include_any_filter);

  /**
   * @brief Returns the filename where the recently opened/saved projects should be stored
   */
  static QString GetRecentProjectsFilePath();

  /**
   * @brief Called only on startup to set the locale
   */
  void SetStartupLocale();

  /**
   * @brief Performs a "save as" on a specific project
   */
  bool SaveProjectAs(Project *p);

  /**
   * @brief Adds a filename to the top of the recently opened projects list (or moves it if it already exists)
   */
  void PushRecentlyOpenedProject(const QString &s);

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
  void SaveProjectInternal(Project *project, const QString &override_filename = QString());

  /**
   * @brief Retrieves the currently most active sequence for exporting
   */
  ViewerOutput *GetSequenceToExport();

  static QString GetAutoRecoveryIndexFilename();

  void SaveUnrecoveredList();

  /**
   * @brief Internal main window object
   */
  MainWindow* main_window_;

  /**
   * @brief List of currently open projects
   */
  QList<Project*> open_projects_;

  /**
   * @brief Currently active tool
   */
  Tool::Item tool_;

  /**
   * @brief Currently active addable object
   */
  Tool::AddableObject addable_object_;

  /**
   * @brief Currently selected transition
   */
  QString selected_transition_;

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
   * @brief Parameters set up in main() determining how the program should run
   */
  CoreParams core_params_;

  /**
   * @brief Static singleton core instance
   */
  static Core* instance_;

  /**
   * @brief Internal translator
   */
  QTranslator* translator_;

  /**
   * @brief List of projects that are unsaved but have autorecovery projects
   */
  QVector<QUuid> autorecovered_projects_;

private slots:
  void SaveAutorecovery();

  void ProjectSaveSucceeded(Task *task);

  /**
   * @brief Adds a project to the "open projects" list
   */
  void AddOpenProject(olive::Project* p);

  bool AddOpenProjectFromTask(Task* task);

  void ImportTaskComplete(Task *task);

  bool ConfirmImageSequence(const QString &filename);

  void ProjectWasModified(bool e);

  bool StartHeadlessExport();

  void OpenStartupProject();

  void AddRecoveryProjectFromTask(Task* task);

  /**
   * @brief Internal project open
   */
  void OpenProjectInternal(const QString& filename, bool recovery_project = false);

};

}

#endif // CORE_H
