/*
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
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
	explicit MainWindow(QWidget *parent = 0);
	void updateTitle(const QString &url);
	virtual ~MainWindow();

	void launch_with_project(const QString& s);

	void make_new_menu(QMenu* parent);
	void make_inout_menu(QMenu* parent);

	void load_shortcuts(const QString &fn, bool first = false);
	void save_shortcuts(const QString &fn);

	QString appName;

public slots:
	void undo();
	void redo();
	void open_speed_dialog();
	void cut();
	void copy();
	void paste();
	void new_project();
	void autorecover_interval();
	void nest();
	void toggle_full_screen();

protected:
	virtual void closeEvent(QCloseEvent *);
	virtual void paintEvent(QPaintEvent *event);

private slots:
	void clear_undo_stack();

	void show_about();
	void delete_slot();
	void select_all();

	void new_sequence();

	void zoom_in();
	void zoom_out();
	void export_dialog();
	void ripple_delete();

	void open_project();
	bool save_project_as();
	bool save_project();

	void go_to_in();
	void go_to_out();
	void go_to_start();
	void prev_frame();
	void playpause();
	void next_frame();
	void go_to_end();
	void prev_cut();
	void next_cut();

	void reset_layout();

	void preferences();

	void zoom_in_tracks();

	void zoom_out_tracks();

	void fileMenu_About_To_Be_Shown();
	void fileMenu_About_To_Hide();
	void editMenu_About_To_Be_Shown();
	void windowMenu_About_To_Be_Shown();
	void viewMenu_About_To_Be_Shown();
	void toolMenu_About_To_Be_Shown();

	void duplicate();

	void add_default_transition();

	void new_folder();

	void load_recent_project();

	void ripple_to_in_point();
	void ripple_to_out_point();

	void set_in_point();
	void set_out_point();

    void clear_in();
    void clear_out();
	void clear_inout();
	void delete_inout();
	void ripple_delete_inout();
    void enable_inout();

	// title safe area functions
	void set_tsa_disable();
	void set_tsa_default();
	void set_tsa_43();
	void set_tsa_169();
	void set_tsa_custom();

	void set_marker();

	void toggle_enable_clips();
	void edit_to_in_point();
	void edit_to_out_point();
	void paste_insert();
	void toggle_bool_action();
	void set_autoscroll();
	void menu_click_button();
	void toggle_panel_visibility();
	void set_timecode_view();
	void open_project_worker(const QString &fn, bool autorecovery);

	void load_with_launch();

	void show_action_search();

private:
	void setup_layout(bool reset);
	bool can_close_project();
	void setup_menus();

	void set_bool_action_checked(QAction* a);
	void set_int_action_checked(QAction* a, const int& i);
	void set_button_action_checked(QAction* a);

	// menu bar menus
	QMenu* window_menu = NULL;

	// file menu actions
	QMenu* open_recent = NULL;
	QAction* clear_open_recent_action = NULL;

	// view menu actions
	QAction* track_lines = NULL;
	QAction* frames_action = NULL;
	QAction* drop_frame_action = NULL;
	QAction* nondrop_frame_action = NULL;
	QAction* milliseconds_action = NULL;
	QAction* no_autoscroll = NULL;
	QAction* page_autoscroll = NULL;
	QAction* smooth_autoscroll = NULL;
	QAction* title_safe_off = NULL;
	QAction* title_safe_default = NULL;
	QAction* title_safe_43 = NULL;
	QAction* title_safe_169 = NULL;
	QAction* title_safe_custom = NULL;
	QAction* full_screen = NULL;
	QAction* show_all = NULL;

	// tool menu actions
	QAction* pointer_tool_action = NULL;
	QAction* edit_tool_action = NULL;
	QAction* ripple_tool_action = NULL;
	QAction* razor_tool_action = NULL;
	QAction* slip_tool_action = NULL;
	QAction* slide_tool_action = NULL;
	QAction* hand_tool_action = NULL;
	QAction* transition_tool_action = NULL;
	QAction* snap_toggle = NULL;
	QAction* selecting_also_seeks = NULL;
	QAction* edit_tool_also_seeks = NULL;
	QAction* edit_tool_selects_links = NULL;
	QAction* seek_to_end_of_pastes = NULL;
	QAction* scroll_wheel_zooms = NULL;
	QAction* rectified_waveforms = NULL;
	QAction* enable_drag_files_to_timeline = NULL;
	QAction* autoscale_by_default = NULL;
	QAction* enable_seek_to_import = NULL;
	QAction* enable_audio_scrubbing = NULL;
	QAction* enable_drop_on_media_to_replace = NULL;
	QAction* enable_hover_focus = NULL;
	QAction* set_name_and_marker = NULL;
	QAction* loop_action = NULL;
	QAction* pause_at_out_point_action = NULL;

	// edit menu actions
	QAction* undo_action = NULL;
	QAction* redo_action = NULL;

	bool enable_launch_with_project = false;
};

extern MainWindow* mainWindow;

#endif // MAINWINDOW_H
