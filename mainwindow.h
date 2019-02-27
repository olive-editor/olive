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

  virtual bool event(QEvent* e) override;

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

  /**
   * @brief Set panel lock
   *
   * @param locked
   *
   * If **TRUE** prevents panels from being moved around. Defaults to **FALSE**.
   */
  void set_panels_locked(bool locked);

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

  void Retranslate();

  // file menu actions
  QMenu* file_menu;
  QMenu* new_menu;
  QAction* open_project;
  QMenu* open_recent;
  QAction* open_action;
  QAction* clear_open_recent_action;
  QAction* save_project;
  QAction* save_project_as;
  QAction* import_action;
  QAction* export_action;
  QAction* exit_action;

  // edit menu actions
  QMenu* edit_menu;
  QAction* undo_action;
  QAction* redo_action;
  QAction* select_all_action;
  QAction* deselect_all_action;
  QAction* ripple_to_in_point_;
  QAction* ripple_to_out_point_;
  QAction* edit_to_in_point_;
  QAction* edit_to_out_point_;
  QAction* delete_inout_point_;
  QAction* ripple_delete_inout_point_;
  QAction* setedit_marker_;


  // view menu actions
  QMenu* view_menu;
  QAction* zoom_in_;
  QAction* zoom_out_;
  QAction* increase_track_height_;
  QAction* decrease_track_height_;
  QAction* track_lines;
  QAction* frames_action;
  QAction* drop_frame_action;
  QAction* nondrop_frame_action;
  QAction* milliseconds_action;
  QAction* no_autoscroll;
  QAction* page_autoscroll;
  QAction* smooth_autoscroll;

  QMenu* title_safe_area_menu;
  QAction* title_safe_off;
  QAction* title_safe_default;
  QAction* title_safe_43;
  QAction* title_safe_169;
  QAction* title_safe_custom;

  QAction* full_screen;
  QAction* full_screen_viewer_;
  QAction* show_all;

  // playback menu
  QMenu* playback_menu;

  QAction* go_to_start_;
  QAction* previous_frame_;
  QAction* playpause_;
  QAction* play_in_to_out_;
  QAction* next_frame_;
  QAction* go_to_end_;
  QAction* go_to_prev_cut_;
  QAction* go_to_next_cut_;
  QAction* go_to_in_point_;
  QAction* go_to_out_point_;
  QAction* shuttle_left_;
  QAction* shuttle_stop_;
  QAction* shuttle_right_;
  QAction* loop_action_;

  // window menu

  QMenu* window_menu;

  QAction* window_project_action;
  QAction* window_effectcontrols_action;
  QAction* window_timeline_action;
  QAction* window_graph_editor_action;
  QAction* window_footageviewer_action;
  QAction* window_sequenceviewer_action;

  QAction* maximize_panel_;
  QAction* lock_panels_;
  QAction* reset_default_layout_;

  // tools menu
  QMenu* tools_menu;

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
  QAction* horizontal_scroll_wheel;
  QAction* rectified_waveforms;
  QAction* enable_drag_files_to_timeline;
  QAction* autoscale_by_default;
  QAction* enable_seek_to_import;
  QAction* enable_audio_scrubbing;
  QAction* enable_drop_on_media_to_replace;
  QAction* enable_hover_focus;
  QAction* set_name_and_marker;
  QAction* seek_also_selects;
  QAction* preferences_action_;
  QAction* clear_undo_action_;

  // help menu
  QMenu* help_menu;
  QAction* action_search_;
  QAction* debug_log_;
  QAction* about_action_;

  // used to store the panel state when one panel is maximized
  QByteArray temp_panel_state;

  // used in paintEvent() to determine the first paintEvent() performed
  bool first_show;
};

namespace olive {
extern MainWindow* MainWindow;
}

#endif // MAINWINDOW_H
