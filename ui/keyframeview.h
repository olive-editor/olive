#ifndef KEYFRAMEVIEW_H
#define KEYFRAMEVIEW_H

#include <QWidget>
#include <QPainter>

struct Clip;
class Effect;
class EffectRow;
class TimelineHeader;

class KeyframeView : public QWidget {
	Q_OBJECT
public:
	KeyframeView(QWidget* parent = 0);

	void delete_selected_keyframes();

	TimelineHeader* header;

	long visible_in;
	long visible_out;
public slots:
	void set_x_scroll(int);
	void set_y_scroll(int);
private:
	long adjust_row_keyframe(EffectRow* row, long time);
	QVector<EffectRow*> selected_rows;
    QVector<int> selected_keyframes;
	QVector<int> rowY;
	long frame_diff;
	QVector<EffectRow*> rows;
	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent *event);
	void paintEvent(QPaintEvent *event);
    void draw_keyframe(QPainter& p, int x, int y, bool darker);
    bool mousedown;
	bool dragging;
	bool keys_selected;
	bool select_rect;

	bool keyframeIsSelected(EffectRow* row, int keyframe);

	long drag_frame_start;
	long last_frame_diff;
	int rect_select_x;
	int rect_select_y;
	int rect_select_w;
	int rect_select_h;

	int x_scroll;
	int y_scroll;
};

#endif // KEYFRAMEVIEW_H
