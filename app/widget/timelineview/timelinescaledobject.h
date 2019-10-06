#ifndef TIMELINESCALEDOBJECT_H
#define TIMELINESCALEDOBJECT_H

#include "common/rational.h"

class TimelineScaledObject
{
public:
  TimelineScaledObject();

protected:
  double TimeToScreenCoord(const rational& time);

  double scale_;

private:

};

#endif // TIMELINESCALEDOBJECT_H
