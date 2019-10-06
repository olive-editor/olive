#include "timelinescaledobject.h"

TimelineScaledObject::TimelineScaledObject() :
  scale_(1.0)
{

}

double TimelineScaledObject::TimeToScreenCoord(const rational &time)
{
  return time.toDouble() * scale_;
}
