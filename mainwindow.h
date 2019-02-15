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

    void updateTitle();

	void load_shortcuts(const QString &fn, bool first = false);
	void save_shortcuts(const QString &fn);

	void load_css_from_file(const QString& fn);

public slots:
    void toggle_full_screen();

signals:
    void finished_first_paint();

protected:
	virtual void closeEvent(QCloseEvent *) override;
	virtual void paintEvent(QPaintEvent *event) override;

private slots:
	void maximize_panel();
	void reset_layout();

    void fileMenu_About_To_Be_Shown();
	void editMenu_About_To_Be_Shown();
	void windowMenu_About_To_Be_Shown();
	void playbackMenu_About_To_Be_Shown();
	void viewMenu_About_To_Be_Shown();
    void toolMenu_About_To_Be_Shown();

    void toggle_panel_visibility();

private:
	void setup_layout(bool reset);
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
