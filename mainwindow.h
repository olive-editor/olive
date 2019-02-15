#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class Project;
class EffectControls;
class Viewer;
class Timeline;

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
    explicit MainWindow(QWidget *parent);
    virtual ~MainWindow() override;

    /**
     * @brief Update window title
     *
     * Updates the window title to reflect the current project filename. Call if the project filename changes.
     *
     * NOTE: It's recommended to use update_project_filename() from Olive::Global to update the filename completely
     * instead of calling this function directly (update_project_filename() calls this function in the process).
     */
    void updateTitle();

    /**
     * @brief Load shortcut file.
     *
     * Loads a shortcut configuration from file and sets Olive to use them.
     *
     * @param fn
     *
     * URL of the shortcut file to be loaded
     *
     */
    void load_shortcuts(const QString &fn);

    /**
     * @brief Save shortcut file.
     *
     * Saves the current shortcut configuration to file. Only saves shortcuts that have been changed from default.
     *
     * @param fn
     *
     * URL to save the shortcut file to.
     */
	void save_shortcuts(const QString &fn);

    /**
     * @brief Load a CSS/QSS style from file to customize Olive's interface.
     *
     * @param fn
     *
     * URL to load the CSS file from.
     */
	void load_css_from_file(const QString& fn);

public slots:
    /**
     * @brief Toggles full screen mode.
     *
     * Toggles the main window between full screen and windowed modes.
     */
    void toggle_full_screen();

signals:
    /**
     * @brief Signal emitted once when the main window has finished initializing
     *
     * Emitted the first time paintEvent runs. Connect this to functions that must be completed post-initialization.
     */
    void finished_first_paint();

protected:
    /**
     * @brief Close event
     *
     * Confirms whether the project can be closed, and if so performs various clean-up functions before the application
     * exits. It's preferable to call clean-up functions here rather than in the destructor because this will get called
     * first.
     */
	virtual void closeEvent(QCloseEvent *) override;

    /**
     * @brief Paint event
     *
     * Overridden to provide the finished_first_paint() signal.
     */
    virtual void paintEvent(QPaintEvent *) override;

private slots:
    /**
     * @brief Maximizes the currently hovered panel.
     *
     * Saves the current state of the panels/dock widgets and removes all except the currently hovered panel,
     * effectively maximizing the panel to the entire window.
     */
	void maximize_panel();

    /**
     * @brief Reset panel layout to default.
     *
     * Resets the current panel layout to default. Doesn't save the current layout.
     */
	void reset_layout();

    /**
     * @brief Function to prepare File menu.
     *
     * Primarily used to populate the Open Recent Projects menu.
     */
    void fileMenu_About_To_Be_Shown();

    /**
     * @brief Function to prepare Edit menu.
     *
     * Primarily used to set the enabled state on Undo and Redo depending if there are undos/redos available.
     */
	void editMenu_About_To_Be_Shown();

    /**
     * @brief Function to prepare Window menu.
     *
     * Primarily used to set the checked state of menu items corresponding to the panels that are currently visible.
     */
	void windowMenu_About_To_Be_Shown();

    /**
     * @brief Function to prepare Playback menu.
     *
     * Primarily used to set the checked state on the "Loop" item.
     */
	void playbackMenu_About_To_Be_Shown();

    /**
     * @brief Function to prepare View menu.
     *
     * Primarily used to set the checked state of various options in the view menu (e.g. title safe area, timecode
     * units, etc.)
     */
	void viewMenu_About_To_Be_Shown();

    /**
     * @brief Function to prepare Tools menu.
     *
     * Primarily used to set the checked state on various settings available from the Tools menu.
     */
    void toolMenu_About_To_Be_Shown();

    /**
     * @brief Toggle whether a panel is visible or not.
     *
     * Assumes the sender() QAction has a pointer to a QDockWidget in its data variable. Casts it and toggles its
     * visibility.
     */
    void toggle_panel_visibility();

private:
    /**
     * @brief Internal function for setting the panel layout to a predetermined preset.
     *
     * Resets layout to default and optionally loads a layout from file. If loading from file, this function will
     * always load from `get_config_path() + "/layout"`.
     *
     * @param reset
     *
     * **TRUE** if this function should just reset the current layout. **FALSE** if it should load from the
     * aforementioned layout file.
     */
	void setup_layout(bool reset);

    /**
     * @brief Initialize menu bar menus and items.
     *
     * Internal initialization function for all menus and menu items in the main window. Called once from the
     * MainWindow() constructor.
     */
	void setup_menus();

	// menu bar menus
	QMenu* window_menu;

	// file menu actions
	QMenu* open_recent;
	QAction* clear_open_recent_action;

	// view menu actions
	QAction* track_lines;
	QAction* frames_action;
	QAction* drop_frame_action;
	QAction* nondrop_frame_action;
	QAction* milliseconds_action;
	QAction* no_autoscroll;
	QAction* page_autoscroll;
	QAction* smooth_autoscroll;
	QAction* title_safe_off;
	QAction* title_safe_default;
	QAction* title_safe_43;
	QAction* title_safe_169;
	QAction* title_safe_custom;
	QAction* full_screen;
	QAction* show_all;

	// tool menu actions
	QAction* pointer_tool_action;
	QAction* edit_tool_action;
	QAction* ripple_tool_action;
	QAction* razor_tool_action;
	QAction* slip_tool_action;
	QAction* slide_tool_action;
	QAction* hand_tool_action;
	QAction* transition_tool_action;
	QAction* snap_toggle;
	QAction* selecting_also_seeks;
	QAction* edit_tool_also_seeks;
	QAction* edit_tool_selects_links;
	QAction* seek_to_end_of_pastes;
	QAction* scroll_wheel_zooms;
	QAction* rectified_waveforms;
	QAction* enable_drag_files_to_timeline;
	QAction* autoscale_by_default;
	QAction* enable_seek_to_import;
	QAction* enable_audio_scrubbing;
	QAction* enable_drop_on_media_to_replace;
	QAction* enable_hover_focus;
	QAction* set_name_and_marker;
	QAction* loop_action;
	QAction* seek_also_selects;

	// edit menu actions
	QAction* undo_action;
	QAction* redo_action;

	// used to store the panel state when one panel is maximized
	QByteArray temp_panel_state;

    // used in paintEvent() to determine the first paintEvent() performed
    bool first_show;
};

namespace Olive {
    extern MainWindow* MainWindow;
}

#endif // MAINWINDOW_H
