#ifndef KEYFRAMEDRAWING_H
#define KEYFRAMEDRAWING_H

#include <QPainter>

#define KEYFRAME_SIZE 6

void draw_keyframe(QPainter &p, int type, int x, int y, bool darker);

#endif // KEYFRAMEDRAWING_H
