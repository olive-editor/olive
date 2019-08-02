#ifndef TIMELINEVIEWPLAYHEADITEM_H
#define TIMELINEVIEWPLAYHEADITEM_H

#include "timelineviewrect.h"

class TimelineViewPlayheadItem : public TimelineViewRect
{
public:
  TimelineViewPlayheadItem(QGraphicsItem* parent = nullptr);

  void SetPlayhead(const int64_t& playhead);

protected:
  virtual void UpdateRect() override;

  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
  int64_t playhead_;
};

#endif // TIMELINEVIEWPLAYHEADITEM_H
