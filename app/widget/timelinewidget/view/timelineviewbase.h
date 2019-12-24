#ifndef TIMELINEVIEWBASE_H
#define TIMELINEVIEWBASE_H

#include <QGraphicsView>

#include "timelineplayhead.h"
#include "widget/timelinewidget/timelinescaledobject.h"

class TimelineViewBase : public QGraphicsView, public TimelineScaledObject
{
  Q_OBJECT
public:
  TimelineViewBase(QWidget* parent = nullptr);

  virtual void drawForeground(QPainter *painter, const QRectF &rect) override;

public slots:
  void SetTimebase(const rational& timebase);

  void SetTime(const int64_t time);

signals:
  void TimeChanged(const int64_t& time);

protected:
  rational GetPlayheadTime();

  bool PlayheadPress(QMouseEvent* event);
  bool PlayheadMove(QMouseEvent* event);
  bool PlayheadRelease(QMouseEvent* event);

private:
  int64_t playhead_;

  TimelinePlayhead playhead_style_;

  double playhead_scene_left_;
  double playhead_scene_right_;

  bool dragging_playhead_;

};

#endif // TIMELINEVIEWBASE_H
