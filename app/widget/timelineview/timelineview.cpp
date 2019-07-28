#include "timelineview.h"

#include "timelineviewclipitem.h"

TimelineView::TimelineView(QWidget *parent) :
  QGraphicsView(parent)
{
}

void TimelineView::AddClip(ClipBlock *clip)
{
  TimelineViewClipItem* clip_item = new TimelineViewClipItem();
  clip_item->SetClip(clip);
  scene_.addItem(clip_item);

  // FIXME: Test code only
  clip_item->setRect(clip->in().ToDouble(), 0, clip->out().ToDouble(), 100);
  // End test code
}

void TimelineView::SetScale(const double &scale)
{
  scale_ = scale;
}

void TimelineView::Clear()
{
  scene_.clear();
}
