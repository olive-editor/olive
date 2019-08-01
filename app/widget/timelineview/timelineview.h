#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QGraphicsView>

#include "node/block/clip/clip.h"

class TimelineView : public QGraphicsView
{
public:
  TimelineView(QWidget* parent);

  void AddClip(ClipBlock* clip);

  void SetScale(const double& scale);

  void SetTimebase(const rational& timebase);

  void Clear();

private:
  QGraphicsScene scene_;

  double scale_;

  rational timebase_;
};

#endif // TIMELINEVIEW_H
