#include "rectangleselect.h"

void draw_selection_rectangle(QPainter& painter, const QRect& rect) {
	painter.setPen(QColor(204, 204, 204));
	painter.setBrush(QColor(0, 0, 0, 32));
	painter.drawRect(rect);
}
