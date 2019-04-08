#include "nodeview.h"

#include <QWheelEvent>
#include <QDebug>

NodeView::NodeView(QGraphicsScene *scene, QWidget *parent) :
  QGraphicsView(scene, parent),
  hand_moving_(false)
{
  setMouseTracking(true);
  setWindowTitle(tr("Node Editor"));
}

void NodeView::mousePressEvent(QMouseEvent *event)
{
  QGraphicsView::mousePressEvent(event);
}

void NodeView::mouseMoveEvent(QMouseEvent *event)
{
  QGraphicsView::mouseMoveEvent(event);
}

void NodeView::mouseReleaseEvent(QMouseEvent *event)
{
}

void NodeView::wheelEvent(QWheelEvent *event)
{
  if (event->angleDelta().y() > 0) {
    scale(0.9, 0.9);
  } else {
    scale(1.1, 1.1);
  }
}
