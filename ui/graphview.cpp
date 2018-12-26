#include "graphview.h"

#include <QPainter>

GraphView::GraphView(QWidget* parent) {}

void GraphView::paintEvent(QPaintEvent *event) {
    QPainter p(this);

    p.setPen(Qt::white);

    p.drawRect(rect());
}
