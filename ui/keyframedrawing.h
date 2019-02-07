#ifndef KEYFRAMEDRAWING_H
#define KEYFRAMEDRAWING_H

#include <QPainter>

#define KEYFRAME_SIZE 6
#define KEYFRAME_COLOR 160

class EffectRow;

void draw_keyframe(QPainter &p, int type, int x, int y, bool darker, int r = KEYFRAME_COLOR, int g = KEYFRAME_COLOR, int b = KEYFRAME_COLOR);
long adjust_row_keyframe(EffectRow* row, long time, long visible_in);

#endif // KEYFRAMEDRAWING_H
