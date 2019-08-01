#include "timelineview.h"

#include "timelineviewclipitem.h"

TimelineView::TimelineView(QWidget *parent) :
  QGraphicsView(parent)
{
  setScene(&scene_);
  setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  setDragMode(RubberBandDrag);
}

void TimelineView::AddClip(ClipBlock *clip)
{
  TimelineViewClipItem* clip_item = new TimelineViewClipItem();
  clip_item->SetClip(clip);
  scene_.addItem(clip_item);

  // FIXME: Test code only
  double framerate = timebase_.ToDouble();
  double clip_item_x = clip->in().ToDouble() / framerate;
  clip_item->setRect(clip_item_x, 0, clip->out().ToDouble() / framerate - clip_item_x - 1, 100);
  // End test code
}

void TimelineView::SetScale(const double &scale)
{
  scale_ = scale;
}

void TimelineView::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;
}

void TimelineView::Clear()
{
  scene_.clear();
}
