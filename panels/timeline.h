#ifndef TIMELINE_H
#define TIMELINE_H

#include "ui/timelinetools.h"
#include <QDockWidget>
#include <QVector>
#include <QTime>
#include <QTimer>

class QPushButton;
class SourceTable;
class ViewerWidget;
struct Sequence;
struct Clip;
struct Media;
struct MediaStream;

struct Ghost {
	Clip* clip;
	long in;
	long out;
	int track;
	long clip_in;

	long old_in;
	long old_out;
	int old_track;
	long old_clip_in;

	// importing variables
	Media* media;
	MediaStream* media_stream;

	// other variables
	long ghost_length;
	long media_length;
};

struct Selection {
	long in;
	long out;
	int track;

	long old_in;
	long old_out;
	int old_track;
};

namespace Ui {
class Timeline;
}

class Timeline : public QDockWidget
{
	Q_OBJECT

public:
	explicit Timeline(QWidget *parent = 0);
	~Timeline();

	bool focused();
	void zoom_in();
	void zoom_out();
    void undo();
    void redo();
    void copy(bool del);
    void paste();
    bool split_selection();
    void split_at_playhead();
    void update_sequence();

    int getScreenPointFromFrame(long frame);
    long getFrameFromScreenPoint(int x);

    void snap_to_clip(long* l);

	long playhead;

	// playback functions
	void go_to_start();
	void previous_frame();
	void next_frame();
	void seek(long p);
	bool toggle_play();
	void play();
	void pause();
	void go_to_end();
	bool playing;
	long playhead_start;
    qint64 start_msecs;
	QTimer playback_updater;

	// shared information
	int tool;
    long cursor_frame;
    int cursor_track;
	float zoom;
	long drag_frame_start;
	int drag_track_start;
	void redraw_all_clips();

    // snapping
    bool snapping;
    bool snapped;
    long snap_point;

	// selecting functions
	bool selecting;
	QVector<Selection> selections;
	bool is_clip_selected(int clip_index);
	void delete_selection(bool ripple);
	void select_all();

	// moving
	bool moving_init;
	bool moving_proc;
	QVector<Ghost> ghosts;

	// trimming
	int trim_target;
	bool trim_in;

	// splitting
	bool splitting;

	// importing
	bool importing;

	// ripple
	void ripple(long ripple_point, long ripple_length);

public slots:
	void repaint_timeline();

private slots:
	void on_toolEditButton_toggled(bool checked);

	void on_toolArrowButton_toggled(bool checked);

	void on_toolRazorButton_toggled(bool checked);

	void on_pushButton_4_clicked();

	void on_pushButton_5_clicked();

	void on_toolRippleButton_toggled(bool checked);

	void on_toolRollingButton_toggled(bool checked);

	void on_toolSlipButton_toggled(bool checked);

    void on_snappingButton_toggled(bool checked);

private:
	Ui::Timeline *ui;
	QVector<QPushButton*> tool_buttons;
	void decheck_tool_buttons(QObject* sender);
	void set_tool(int tool);
	long last_frame;
    QVector<Clip*> clip_clipboard;
};

#endif // TIMELINE_H
