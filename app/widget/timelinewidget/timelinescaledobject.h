#ifndef TIMELINESCALEDOBJECT_H
#define TIMELINESCALEDOBJECT_H

#include "common/rational.h"

class TimelineScaledObject
{
public:
  TimelineScaledObject();

  const rational& timebase();
  const double& timebase_dbl();

protected:
  double TimeToScene(const rational& time);
  rational SceneToTime(const double &x);

  void SetTimebaseInternal(const rational& timebase);

  double scale_;

private:

  rational timebase_;

  double timebase_dbl_;
};

#endif // TIMELINESCALEDOBJECT_H
