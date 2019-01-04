#ifndef KEYFRAMEDRAWING_H
#define KEYFRAMEDRAWING_H

#include <QPainter>

#define KEYFRAME_SIZE 6
#define KEYFRAME_COLOR 160

void draw_keyframe(QPainter &p, int type, int x, int y, bool darker, int r = KEYFRAME_COLOR, int g = KEYFRAME_COLOR, int b = KEYFRAME_COLOR);

#endif // KEYFRAMEDRAWING_H
