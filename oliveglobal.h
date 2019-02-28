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

#ifndef OLIVEGLOBAL_H
#define OLIVEGLOBAL_H

#include <memory>
#include "project/undo.h"

#include <QTimer>
#include <QFile>
#include <QTranslator>

/**
 * @brief The Olive Global class
 *
 * A resource for various global functions used throughout Olive.
 */
class OliveGlobal : public QObject {
    Q_OBJECT
public:
    /**
     * @brief OliveGlobal Constructor
     *
     * Creates Olive Global object. Also sets some default runtime settings and the application name.
     */
    OliveGlobal();

    /**
     * @brief Returns the file dialog filter used when interfacing with Olive project files.
     *
     * @return The file filter string used by QFileDialog to limit the files shown to Olive (*.ove) files.
     */
    const QString& get_project_file_filter();

    /**
     * @brief Change the current active project filename
     *
     * Triggered to change the current active project filename. Call this before calling any internal project
     * saving or loading functions in order to set which file to work with (OliveGlobal::open_project() and
     * OliveGlobal::save_project_as() do this automatically). Also updates the main window title to reflect the
     * project filename.
     *
     * @param s
     *
     * The URL of the project file to work with. Can be an empty string, in which case Olive will treat the project
     * as an unsaved project.
     */
    void update_project_filename(const QString& s);

    /**
     * @brief Check whether an auto-recovery file exists and ask the user if they want to load it.
     *
     * Usually called on initialization. Checks if an auto-recovery file exists (meaning the last session of Olive
     * didn't close correctly). If it finds one, asks the user if they want to load it. If so, loads the auto-recovery
     * project.
     */
    void check_for_autorecovery_file();

    /**
     * @brief Set the application state depending on if the user is exporting a video
     *
     * Some background functions shouldn't run while Olive is exporting a video. This function will disable/enable them
     * as necessary.
     *
     * The current functions are as follows:
     * * Auto-recovery interval. Olive saves an auto-recovery just before exporting anyway and seeing as the user
     * cannot make changes while rendering, there's no reason to continue saving auto-recovery files.
     * * Audio device playback. Olive uses the same internal audio buffer for exporting as it does for playback, but
     * this buffer does not need to be forwarded to the output device when exporting.
     *
     * @param rendering
     *
     * **TRUE** if Olive is about to export a video. **FALSE** if Olive has finished exporting.
     */
    void set_rendering_state(bool rendering);

    /**
     * @brief Set a project to load just after launching
     *
     * Called by main() if Olive was called with a project file as a running argument. Sets up Olive to load the
     * specified project once its finished initializing.
     *
     * @param s
     *
     * The URL of the project file to load.
     */
    void load_project_on_launch(const QString& s);

    /**
     * @brief Retrieves the URL of the config file containing the autorecovery projects
     * @return The URL as a string
     */
    QString get_recent_project_list_file();

    /**
     * @brief (Re)load translation file from olive::config
     */
    void load_translation_from_config();

    /**
     * @brief Converts an SVG into a QIcon with a semi-transparent for the QIcon::Disabled property
     *
     * @param path
     *
     * Path to SVG file
     */
    static QIcon CreateIconFromSVG(const QString& path);

public slots:
    /**
     * @brief Undo user's last action
     */
    void undo();

    /**
     * @brief Redo user's last action
     */
    void redo();

    /**
     * @brief Paste contents of clipboard
     *
     * Pastes contents of clipboard. Seeing as several types of data can be copied into the clipboard, this
     * function will automatically determine what type of data is in the clipboard and paste it in the correct
     * location (e.g. clip data will go to the Timeline, effect data will go to Effect Controls).
     */
    void paste();

    /**
     * @brief Paste contents of clipboard, making space for it when possible
     *
     * Pastes contents of clipboard (same as paste()). If the clipboard contains clip data, the clips are cut at the
     * current playhead and ripple forward to make space for the clips in the clipboard. Can be considered
     * semi-non-destructive as a result (as opposed to paste() overwriting clips). If the clipboard contains effect
     * data, the functionality is identical to paste().
     */
    void paste_insert();

    /**
     * @brief Create new project.
     *
     * Confirms whether the current project can be closed, and if so, clears all current project data and resets
     * program state. Standard `File > New` behavior.
     */
    void new_project();

    /**
     * @brief Open a project from file.
     *
     * Confirms whether the current project can be closed, and if so, shows an open file dialog to allow the user to
     * select a project file and then triggers a project load with it.
     */
    void open_project();

    /**
     * @brief Open recent project from list
     *
     * Triggers a project load from the internal recent projects list.
     *
     * @param index
     *
     * Index in the list of the project fille to load
     */
    void open_recent(int index);

    /**
     * @brief Shows a save file dialog and saves the project as the resulting filename
     *
     * Shows a save file dialog for the user to save their current project as a different filename from the current
     * one. Also triggered by save_project() if the file hasn't been saved yet.
     *
     * @return **TRUE** if the user saved the project. **FALSE** if they cancelled out of the save file dialog. Useful
     * if a user is closing an unsaved project, clicks "Yes" to save, we know if they actually saved or not and won't
     * continue closing the project if they didn't.
     */
    bool save_project_as();

    /**
     * @brief Saves the current project to file
     *
     * If the project has been saved already, this function will overwrite the project file with the current project
     * data. Calls save_project_as() if the file has not been saved before.
     *
     * @return **TRUE** if the project has been saved before and was successfully overwritten. Otherwise returns the
     * value of save_project_as(). Useful if the user closing an unsaved project, clicks "Yes" to save, we know if they
     * actually saved or not and won't continue closing the project if they didn't.
     */
    bool save_project();

    /**
     * @brief Determine whether the current project can be closed.
     *
     * Queried any time the current project is going to be closed (e.g. starting a new project, loading a project,
     * exiting Olive, etc.) If the project has unsaved changes, this function asks the user whether they want to save or
     * not. If the user does, calls save_project() (which may in turn call save_project_as() if the project has never
     * been saved).
     *
     * @return **TRUE** if the project can be closed. FALSE if not. If the project does NOT have unsaved changes, always
     * returns **TRUE**. If it does and the user clicks YES, this returns the result of save_project(). If the user
     * clicks NO, this returns **TRUE**. If the user clicks CANCEL, this returns **FALSE**.
     */
    bool can_close_project();

    /**
     * @brief Open the Export dialog to trigger an export of the current sequence.
     */
    void open_export_dialog();

    /**
     * @brief Open the About Olive dialog.
     */
    void open_about_dialog();

    /**
     * @brief Open the Debug Log window.
     */
    void open_debug_log();

    /**
     * @brief Open the Speed/Duration dialog.
     */
    void open_speed_dialog();

    /**
     * @brief Open the Action Search overlay.
     */
    void open_action_search();

    /**
     * @brief Clears the current undo stack.
     *
     * Clears all current commands in the undo stack. Mostly used for debugging.
     */
    void clear_undo_stack();

    /**
     * @brief Function called when Olive has finished starting up
     *
     * Sets up some last things for Olive that must be run after Olive has completed initialization. If a project was
     * loaded as a command line argument, it's loaded here.
     */
    void finished_initialize();

    /**
     * @brief Save an auto-recovery file of the current project.
     *
     * Call this function to save the current state of the project as an auto-recovery project. Called regularly by
     * `autorecovery_timer`.
     */
    void save_autorecovery_file();

    /**
     * @brief Opens the Preferences dialog
     */
    void open_preferences();

    /**
     * @brief Set the current active Sequence
     *
     * Call this to change the active Sequence (e.g. when the user double clicks a Sequence in the Project panel).
     * This will affect panel_timeline, panel_sequence_viewer, and panel_effect_controls and can then be retrieved
     * using olive::ActiveSequence.
     *
     * @param s
     *
     * The Sequence to set as the active Sequence.
     */
    void set_sequence(SequencePtr s);

private:
    /**
     * @brief Internal function to handle loading a project from file
     *
     * Start loading a project. Doesn't check if the current project can be closed, doesn't check if the project exists.
     * In most cases, you'll want open_project() to be end-user friendly.
     *
     * @param fn
     *
     * The URL to the project to load.
     *
     * @param autorecovery
     *
     * Whether this file is an autorecovery file. If it is, after the load Olive will set the project URL to a new file
     * beside the original project file so that it does not overwrite the original and so that the user is not working
     * on the autorecovery project in Olive's application data directory.
     */
    void open_project_worker(const QString& fn, bool autorecovery);

    /**
     * @brief File filter used for any file dialogs relating to Olive project files.
     */
    QString project_file_filter;

    /**
     * @brief Regular interval to save an auto-recovery project.
     */
    QTimer autorecovery_timer;

    /**
     * @brief Internal variable set to **TRUE** by main() if a project file was set as an argument
     */
    bool enable_load_project_on_init;

    /**
     * @brief Internal translator object that interfaces with the currently loaded language file
     */
    std::unique_ptr<QTranslator> translator;

private slots:

};

namespace olive {
    /**
     * @brief Object resource for various global functions used throughout Olive
     */
    extern std::unique_ptr<OliveGlobal> Global;

    /**
     * @brief Currently active project filename
     *
     * Filename for the currently active project. Empty means the file has not
     * been saved yet.
     */
    extern QString ActiveProjectFilename;

    /**
     * @brief Current application name
     */
    extern QString AppName;
}

#endif // OLIVEGLOBAL_H
