#ifndef TIMELINEVIEWPLAYHEADITEM_H
#define TIMELINEVIEWPLAYHEADITEM_H

#include "timelineplayhead.h"
#include "timelineviewrect.h"

class TimelineViewPlayheadItem : public TimelineViewRect
{
public:
  TimelineViewPlayheadItem(QGraphicsItem* parent = nullptr);

  void SetPlayhead(const int64_t& playhead);

  void SetTimebase(const rational& timebase);

protected:
  virtual void UpdateRect() override;

  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
  int64_t playhead_;

  rational timebase_;

  TimelinePlayhead style_;
};

#endif // TIMELINEVIEWPLAYHEADITEM_H
