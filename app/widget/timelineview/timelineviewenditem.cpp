#include "timelineviewenditem.h"

TimelineViewEndItem::TimelineViewEndItem(QGraphicsItem *parent) :
  TimelineViewRect(parent),
  end_padding_(0)
{
}

void TimelineViewEndItem::SetEndTime(const rational &time)
{
  end_time_ = time;

  UpdateRect();
}

void TimelineViewEndItem::SetEndPadding(int padding)
{
  end_padding_ = padding;

  UpdateRect();
}

void TimelineViewEndItem::UpdateRect()
{
  // Doesn't need to be more than one pixel
  setRect(0, 0, 1, 1);

  setPos(TimeToScene(end_time_) + end_padding_, 0);
}

void TimelineViewEndItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  // Item is invisible, this is a no-op
  Q_UNUSED(painter)
  Q_UNUSED(option)
  Q_UNUSED(widget)
}
