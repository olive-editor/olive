#ifndef KEYFRAMEVIEW_H
#define KEYFRAMEVIEW_H

#include <QWidget>
#include <QPainter>

class Clip;
class Effect;
class EffectRow;
class EffectField;
class TimelineHeader;

class KeyframeView : public QWidget {
	Q_OBJECT
public:
	KeyframeView(QWidget* parent = 0);

	void delete_selected_keyframes();

	TimelineHeader* header;

	long visible_in;
	long visible_out;
signals:
    void wheel_event_signal(QWheelEvent*);
public slots:
	void set_x_scroll(int);
	void set_y_scroll(int);
	void resize_move(double d);
private:
	QVector<EffectField*> selected_fields;
	QVector<int> selected_keyframes;
	QVector<int> rowY;
	QVector<EffectRow*> rows;
	QVector<long> old_key_vals;
	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent *event);
	void paintEvent(QPaintEvent *event);
    void wheelEvent(QWheelEvent* e);
	bool mousedown;
	bool dragging;
	bool keys_selected;
	bool select_rect;
	bool scroll_drag;

	bool keyframeIsSelected(EffectField *field, int keyframe);

	long drag_frame_start;
	long last_frame_diff;
	int rect_select_x;
	int rect_select_y;
	int rect_select_w;
	int rect_select_h;
	int rect_select_offset;

	int x_scroll;
	int y_scroll;

	void update_keys();
private slots:
	void show_context_menu(const QPoint& pos);
	void menu_set_key_type(QAction*);
};

#endif // KEYFRAMEVIEW_H
