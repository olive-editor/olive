#include "keyframeview.h"

KeyframeView::KeyframeView(QWidget *parent) :
  KeyframeViewBase(parent)
{
  setAlignment(Qt::AlignLeft | Qt::AlignTop);
}

void KeyframeView::wheelEvent(QWheelEvent *event)
{
  if (!HandleZoomFromScroll(event)) {
    KeyframeViewBase::wheelEvent(event);
  }
}

void KeyframeView::AddKeyframe(NodeKeyframePtr key, int y)
{
  QPoint global_pt(0, y);
  QPoint local_pt = mapFromGlobal(global_pt);
  QPointF scene_pt = mapToScene(local_pt);

  KeyframeViewItem* item = AddKeyframeInternal(key);
  item->SetOverrideY(scene_pt.y());
}
