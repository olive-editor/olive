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
	explicit MainWindow(QWidget *parent, const QString& an);
	void updateTitle(const QString &url);
	~MainWindow();

	void launch_with_project(const QString& s);

	void make_new_menu(QMenu* parent);
	void make_inout_menu(QMenu* parent);

	void load_shortcuts(const QString &fn, bool first = false);
	void save_shortcuts(const QString &fn);

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
	void closeEvent(QCloseEvent *);
	void paintEvent(QPaintEvent *event);

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
	QAction* pause_at_out_point_action;

	// edit menu actions
	QAction* undo_action;
	QAction* redo_action;

	void set_bool_action_checked(QAction* a);
	void set_int_action_checked(QAction* a, const int& i);
	void set_button_action_checked(QAction* a);

	bool enable_launch_with_project;

	QString appName;
};

extern MainWindow* mainWindow;

#endif // MAINWINDOW_H
