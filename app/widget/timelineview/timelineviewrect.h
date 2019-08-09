#ifndef TIMELINEVIEWRECT_H
#define TIMELINEVIEWRECT_H

#include <QGraphicsRectItem>

#include "common/rational.h"

class TimelineViewRect : public QGraphicsRectItem
{
public:
  TimelineViewRect(QGraphicsItem* parent = nullptr);

  void SetScale(const double& scale);

protected:
  virtual void UpdateRect() = 0;

  double TimeToScreenCoord(const rational& time);

  double scale_;
};

#endif // TIMELINEVIEWRECT_H
