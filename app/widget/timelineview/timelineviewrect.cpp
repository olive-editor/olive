#include "timelineviewrect.h"

TimelineViewRect::TimelineViewRect(QGraphicsItem* parent) :
  QGraphicsRectItem(parent),
  scale_(1.0)
{

}

void TimelineViewRect::SetScale(const double &scale)
{
  scale_ = scale;

  UpdateRect();
}

double TimelineViewRect::TimeToScreenCoord(const rational &time)
{
  return time.toDouble() * scale_;
}
