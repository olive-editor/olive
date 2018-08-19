#ifndef KEYFRAMEVIEW_H
#define KEYFRAMEVIEW_H

#include <QWidget>
#include <QPainter>

struct Clip;
class Effect;
class EffectRow;

class KeyframeView : public QWidget {
	Q_OBJECT
public:
	KeyframeView(QWidget* parent = 0);
	QVector<Effect*> effects;

	long visible_in;
	long visible_out;
private:
	QVector<int> rowY;
	QVector<EffectRow*> rows;
	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent *event);
	void paintEvent(QPaintEvent *event);
	void draw_keyframe(QPainter& p, int x, int y, bool semiTransparent);
	bool enable_reload;
	bool mouseover;
	long mouseover_frame;
	int mouseover_row;
};

#endif // KEYFRAMEVIEW_H
