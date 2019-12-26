#include "keyframeview.h"

#include <QVBoxLayout>

KeyframeView::KeyframeView(QWidget *parent) :
  TimelineViewBase(parent)
{
  setBackgroundRole(QPalette::Base);
  setAlignment(Qt::AlignLeft | Qt::AlignTop);
  setDragMode(QGraphicsView::RubberBandDrag);
}

void KeyframeView::AddKeyframe(NodeKeyframePtr key, int y)
{
  QPoint global_pt(0, y);
  QPoint local_pt = mapFromGlobal(global_pt);
  QPointF scene_pt = mapToScene(local_pt);

  KeyframeViewItem* item = new KeyframeViewItem(key, scene_pt.y());
  item->SetScale(scale_);
  item_map_.insert(key.get(), item);
  scene()->addItem(item);
}

void KeyframeView::RemoveKeyframe(NodeKeyframePtr key)
{
  delete item_map_.value(key.get());
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

void KeyframeView::ScaleChangedEvent(double scale)
{
  QMap<NodeKeyframe*, KeyframeViewItem*>::const_iterator iterator;

  for (iterator=item_map_.begin();iterator!=item_map_.end();iterator++) {
    iterator.value()->SetScale(scale);
  }
}
