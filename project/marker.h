#ifndef MARKER_H
#define MARKER_H

#define MARKER_SIZE 4

#include <QString>
#include <QPainter>

struct Marker {
    long frame;
    QString name;
};

void draw_marker(QPainter& p, int x, int y, int bottom, bool selected, bool flipped);

#endif // MARKER_H
