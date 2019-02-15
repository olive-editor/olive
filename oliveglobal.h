#ifndef OLIVEGLOBAL_H
#define OLIVEGLOBAL_H

#include "project/undo.h"

#include <QTimer>
#include <QFile>

class OliveGlobal : public QObject {
    Q_OBJECT
public:
    OliveGlobal();

    const QString& get_project_file_filter();

    void update_project_filename(const QString& s);

    void check_for_autorecovery_file();

    void set_rendering_state(bool rendering);

    void load_project_on_launch(const QString& s);

    QString get_recent_project_list_file();

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


    void new_project();
    void open_project();
    void open_recent();
    bool save_project_as();
    bool save_project();

    bool can_close_project();

    void open_export_dialog();
    void open_about_dialog();
    void open_debug_log();
    void open_speed_dialog();
    void open_action_search();

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

private:
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


private slots:

};

namespace Olive {
    /**
     * @brief Object resource for various global functions used throughout Olive
     */
    extern QSharedPointer<OliveGlobal> Global;

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
