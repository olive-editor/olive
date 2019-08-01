#ifndef TIMELINEVIEWCLIPITEM_H
#define TIMELINEVIEWCLIPITEM_H

#include <QGraphicsRectItem>

#include "node/block/clip/clip.h"

class TimelineViewClipItem : public QGraphicsRectItem
{
public:
  TimelineViewClipItem(QGraphicsItem* parent = nullptr);

  void SetClip(ClipBlock* clip);

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
  ClipBlock* clip_;
};

#endif // TIMELINEVIEWCLIPITEM_H
