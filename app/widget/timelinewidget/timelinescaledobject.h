#ifndef TIMELINESCALEDOBJECT_H
#define TIMELINESCALEDOBJECT_H

#include "common/rational.h"

class TimelineScaledObject
{
public:
  TimelineScaledObject();
  virtual ~TimelineScaledObject() = default;

  void SetTimebase(const rational &timebase);

  const rational& timebase() const;
  const double& timebase_dbl() const;

  static rational SceneToTime(const double &x, const double& x_scale, const rational& timebase, bool round = false);

  const double& GetScale() const;

  void SetScale(const double& scale);

protected:
  double TimeToScene(const rational& time);
  rational SceneToTime(const double &x, bool round = false);

  virtual void TimebaseChangedEvent(const rational&){}

  virtual void ScaleChangedEvent(const double&){}

  void SetMaximumScale(const double& max);

private:
  rational timebase_;

  double timebase_dbl_;

  double scale_;

  double max_scale_;

};

#endif // TIMELINESCALEDOBJECT_H
