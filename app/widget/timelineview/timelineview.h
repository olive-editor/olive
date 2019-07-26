#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QGraphicsView>

#include "project/item/sequence/sequence.h"

class TimelineView : public QGraphicsView
{
public:
  TimelineView(QWidget* parent);

  void SetSequence(Sequence* s);

private:
  Sequence* sequence_;

  QGraphicsScene scene_;
};

#endif // TIMELINEVIEW_H
