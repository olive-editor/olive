#include "keyframeview.h"

#include <QVBoxLayout>

KeyframeView::KeyframeView(QWidget *parent) :
  TimelineViewBase(parent)
{
  setBackgroundRole(QPalette::Base);
  setAlignment(Qt::AlignLeft | Qt::AlignTop);
  setDragMode(QGraphicsView::RubberBandDrag);
}

void KeyframeView::mousePressEvent(QMouseEvent *event)
{
  if (PlayheadPress(event)) {
    return;
  }

  QGraphicsView::mousePressEvent(event);
}

void KeyframeView::mouseMoveEvent(QMouseEvent *event)
{
  if (PlayheadMove(event)) {
    return;
  }

  QGraphicsView::mouseMoveEvent(event);
}

void KeyframeView::mouseReleaseEvent(QMouseEvent *event)
{
  if (PlayheadRelease(event)) {
    return;
  }

  QGraphicsView::mouseReleaseEvent(event);
}
