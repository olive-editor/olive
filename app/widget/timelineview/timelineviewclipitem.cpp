#include "timelineviewclipitem.h"

TimelineViewClipItem::TimelineViewClipItem(QGraphicsItem* parent) :
  QGraphicsRectItem(parent),
  clip_(nullptr)
{
  
}

void TimelineViewClipItem::SetClip(ClipBlock *clip)
{
  clip_ = clip;
}
