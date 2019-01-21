#include "keyframedrawing.h"

#include "project/effect.h"

#define KEYFRAME_POINT_COUNT 4

void draw_keyframe(QPainter &p, int type, int x, int y, bool darker, int r, int g, int b) {
	if (darker) {
		r *= 0.625;
		g *= 0.625;
		b *= 0.625;
	}
	p.setPen(QColor(0, 0, 0));
	p.setBrush(QColor(r, g, b));

	switch (type) {
	case EFFECT_KEYFRAME_LINEAR:
	{
		QPoint points[KEYFRAME_POINT_COUNT] = {QPoint(x-KEYFRAME_SIZE, y), QPoint(x, y-KEYFRAME_SIZE), QPoint(x+KEYFRAME_SIZE, y), QPoint(x, y+KEYFRAME_SIZE)};
		p.drawPolygon(points, KEYFRAME_POINT_COUNT);
	}
		break;
	case EFFECT_KEYFRAME_BEZIER:
		p.drawEllipse(QPoint(x, y), KEYFRAME_SIZE, KEYFRAME_SIZE);
		break;
	case EFFECT_KEYFRAME_HOLD:
		p.drawRect(QRect(x - KEYFRAME_SIZE, y - KEYFRAME_SIZE, KEYFRAME_SIZE*2, KEYFRAME_SIZE*2));
		break;
	}

	p.setBrush(Qt::NoBrush);
}
