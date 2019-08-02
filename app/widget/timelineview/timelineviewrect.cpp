#include "timelineviewrect.h"

TimelineViewRect::TimelineViewRect(QGraphicsItem* parent) :
  QGraphicsRectItem(parent),
  scale_(1.0)
{

}

void TimelineViewRect::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;
  timebase_dbl_ = timebase_.ToDouble();

  UpdateRect();
}

void TimelineViewRect::SetScale(const double &scale)
{
  scale_ = scale;
  UpdateRect();
}

bool TimelineViewRect::TimebaseIsValid()
{
  return timebase_.denominator() != 0;
}

double TimelineViewRect::TimeToScreenCoord(const rational &time)
{
  return time.ToDouble() / timebase_dbl_ * scale_;
}
