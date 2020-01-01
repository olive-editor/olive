#ifndef TIMELINESCALEDOBJECT_H
#define TIMELINESCALEDOBJECT_H

#include "common/rational.h"

class TimelineScaledObject
{
public:
  TimelineScaledObject();

  const rational& timebase();
  const double& timebase_dbl();

  static rational SceneToTime(const double &x, const double& x_scale, const rational& timebase, bool round = false);

protected:
  double TimeToScene(const rational& time);
  rational SceneToTime(const double &x, bool round = false);

  void SetTimebaseInternal(const rational& timebase);

  double scale_;

private:

  rational timebase_;

  double timebase_dbl_;
};

#endif // TIMELINESCALEDOBJECT_H
