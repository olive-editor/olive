#ifndef TIMELINEWIDGET_H
#define TIMELINEWIDGET_H

#include <QTimer>
#include <QWidget>
#include "timelinetools.h"

#define GHOST_THICKNESS 2 // thiccccc
#define CLIP_TEXT_PADDING 3

#define TRACK_MIN_HEIGHT 30
#define TRACK_HEIGHT_INCREMENT 10

struct Sequence;
struct Clip;
class Timeline;
class TimelineAction;
class QScrollArea;
class SetSelectionsCommand;

bool same_sign(int a, int b);

class TimelineWidget : public QWidget {
	Q_OBJECT
public:
	explicit TimelineWidget(QWidget *parent = 0);

	bool bottom_align;

	QScrollArea* container;
protected:
	void paintEvent(QPaintEvent*);

    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void leaveEvent(QEvent *event);

	void dragEnterEvent(QDragEnterEvent *event);
	void dragLeaveEvent(QDragLeaveEvent *event);
	void dropEvent(QDropEvent* event);
	void dragMoveEvent(QDragMoveEvent *event);
private:
	void init_ghosts();
	void update_ghosts(QPoint& mouse_pos);
    bool is_track_visible(int track);
    int getTrackFromScreenPoint(int y);
	int getScreenPointFromTrack(int track);
	int getClipIndexFromCoords(long frame, int track);

    int track_resize_mouse_cache;
    int track_resize_old_value;
    bool track_resizing;
    int track_target;

    QVector<Clip*> pre_clips;
	QVector<Clip*> post_clips;

    int predicted_video_width;
    int predicted_video_height;
    double predicted_new_frame_rate;
    int predicted_audio_freq;
	int predicted_audio_layout;

	// used for "right click ripple"
	long rc_ripple_min;
	long rc_ripple_max;

    QTimer tooltip_timer;
    int tooltip_clip;

    SetSelectionsCommand* selection_command;
signals:

public slots:

private slots:
	void right_click_ripple();
    void show_context_menu(const QPoint& pos);
    void toggle_autoscale();
    void tooltip_timer_timeout();
};

#endif // TIMELINEWIDGET_H
