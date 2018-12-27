#include "graphview.h"

#include <QPainter>

GraphView::GraphView(QWidget* parent) {}

void GraphView::paintEvent(QPaintEvent *event) {
    QPainter p(this);

    p.setPen(Qt::white);

    QRect border = rect();
    border.setWidth(border.width()-1);
    border.setHeight(border.height()-1);
    p.drawRect(border);
}
