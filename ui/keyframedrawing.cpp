/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "keyframedrawing.h"

#include "project/effect.h"
#include "project/clip.h"

#define KEYFRAME_POINT_COUNT 4

// routine for drawing a keyframe onscreen
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

// adjusts keyframe's internal time (in clip time) to timeline time
long adjust_row_keyframe(EffectRow* row, long time, long visible_in) {
    return time
        - row->parent_effect->parent_clip->clip_in()
        + (row->parent_effect->parent_clip->timeline_in() - visible_in);
}
