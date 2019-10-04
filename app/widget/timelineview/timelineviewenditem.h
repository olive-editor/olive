#ifndef TIMELINEVIEWENDITEM_H
#define TIMELINEVIEWENDITEM_H

#include "timelineviewrect.h"

/**
 * @brief An item placed at the end point of the Timeline to ensure the correct scene size
 */
class TimelineViewEndItem : public TimelineViewRect
{
public:
  TimelineViewEndItem(QGraphicsItem* parent = nullptr);

  void SetEndTime(const rational& time);

  void SetEndPadding(int padding);

  virtual void UpdateRect() override;

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
  rational end_time_;

  int end_padding_;
};

#endif // TIMELINEVIEWENDITEM_H
