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
	QVector<Effect*> effects;

	TimelineHeader* header;

	long visible_in;
	long visible_out;
private:
    QVector<int> selected_rows;
    QVector<int> selected_keyframes;
	QVector<int> rowY;
	QVector<EffectRow*> rows;
	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent *event);
	void paintEvent(QPaintEvent *event);
    void draw_keyframe(QPainter& p, int x, int y, bool darker);
    bool mousedown;

    int drag_row_index;
    int drag_keyframe_index;
};

#endif // KEYFRAMEVIEW_H
