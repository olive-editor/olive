#ifndef TIMELINEVIEWRECT_H
#define TIMELINEVIEWRECT_H

#include <QGraphicsRectItem>

#include "common/rational.h"

class TimelineViewRect : public QGraphicsRectItem
{
public:
  TimelineViewRect(QGraphicsItem* parent = nullptr);

  void SetTimebase(const rational& timebase);

  void SetScale(const double& scale);

protected:
  virtual void UpdateRect() = 0;

  bool TimebaseIsValid();

  double TimeToScreenCoord(const rational& time);

  rational timebase_;

  double timebase_dbl_;

  double scale_;
};

#endif // TIMELINEVIEWRECT_H
