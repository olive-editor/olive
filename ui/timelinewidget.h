#ifndef TIMELINEWIDGET_H
#define TIMELINEWIDGET_H

#include <QWidget>
#include "timeline-tools.h"

#define GHOST_THICKNESS 2 // thiccccc
#define CLIP_TEXT_PADDING 3

struct Sequence;
struct Clip;
class Timeline;

class TimelineWidget : public QWidget
{
	Q_OBJECT
public:
	explicit TimelineWidget(QWidget *parent = nullptr);

	bool bottom_align;

	void redraw_clips();
protected:
	void paintEvent(QPaintEvent*) override;

	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;

	void dragEnterEvent(QDragEnterEvent *event);
	void dragLeaveEvent(QDragLeaveEvent *event);
	void dropEvent(QDropEvent* event);
	void dragMoveEvent(QDragMoveEvent *event);
private:
	void init_ghosts();
	void update_ghosts(QPoint& mouse_pos);
	bool is_track_visible(int track);
	long getFrameFromScreenPoint(int x, bool floor);
	int getTrackFromScreenPoint(int y);
	int getScreenPointFromFrame(long frame);
	int getScreenPointFromTrack(int track);
	int getClipIndexFromCoords(long frame, int track);
	int track_height;

	QList<Clip*> pre_clips;
	QList<Clip*> post_clips;

	QPixmap* clip_pixmap;
//	QPixmap selection_pixmap;

signals:

public slots:
};

#endif // TIMELINEWIDGET_H
