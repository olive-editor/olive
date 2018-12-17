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
struct FootageStream;
class Timeline;
class TimelineAction;
class QScrollBar;
class SetSelectionsCommand;
class QPainter;

bool same_sign(int a, int b);
void draw_waveform(Clip* clip, FootageStream* ms, long media_length, QPainter* p, const QRect& clip_rect, int waveform_start, int waveform_limit, double zoom);

class TimelineWidget : public QWidget {
	Q_OBJECT
public:
	explicit TimelineWidget(QWidget *parent = 0);
	QScrollBar* scrollBar;
	bool bottom_align;
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

	void wheelEvent(QWheelEvent *event);
private:
	void init_ghosts();
    void update_ghosts(const QPoint& mouse_pos, bool lock_frame);
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

	Sequence* self_created_sequence;

	// used for "right click ripple"
	long rc_ripple_min;
	long rc_ripple_max;
	void* rc_reveal_media;

    QTimer tooltip_timer;
    int tooltip_clip;

	int scroll;

	SetSelectionsCommand* selection_command;
signals:

public slots:
	void setScroll(int);

private slots:
	void reveal_media();
	void right_click_ripple();
    void show_context_menu(const QPoint& pos);
    void toggle_autoscale();
    void tooltip_timer_timeout();
	void rename_clip();
    void show_stabilizer_diag();
};

#endif // TIMELINEWIDGET_H
