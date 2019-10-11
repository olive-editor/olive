#include "timelinescaledobject.h"

TimelineScaledObject::TimelineScaledObject() :
  scale_(1.0)
{

}

const rational &TimelineScaledObject::timebase()
{
  return timebase_;
}

const double &TimelineScaledObject::timebase_dbl()
{
  return timebase_dbl_;
}

double TimelineScaledObject::TimeToScene(const rational &time)
{
  return time.toDouble() * scale_;
}

rational TimelineScaledObject::SceneToTime(const double &x)
{
  // Adjust screen point by scale and timebase
  qint64 scaled_x_mvmt = qRound64(x / scale_ / timebase_dbl_);

  // Return a time in the timebase
  return rational(scaled_x_mvmt * timebase_.numerator(), timebase_.denominator());
}

void TimelineScaledObject::SetTimebaseInternal(const rational &timebase)
{
  timebase_ = timebase;
  timebase_dbl_ = timebase_.toDouble();
}
