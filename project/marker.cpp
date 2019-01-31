#include "marker.h"

void draw_marker(QPainter &p, int x, int y, int bottom, bool selected, bool flipped) {
    const QPoint points[5] = {
        QPoint(x, bottom),
        QPoint(x + MARKER_SIZE, bottom - MARKER_SIZE),
        QPoint(x + MARKER_SIZE, y),
        QPoint(x - MARKER_SIZE, y),
        QPoint(x - MARKER_SIZE, bottom - MARKER_SIZE)
    };
    p.setPen(Qt::black);
    if (selected) {
        p.setBrush(QColor(208, 255, 208));
    } else {
        p.setBrush(QColor(128, 224, 128));
    }
    p.drawPolygon(points, 5);
}
