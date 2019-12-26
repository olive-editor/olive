#ifndef TIMELINEVIEWBASE_H
#define TIMELINEVIEWBASE_H

#include <QGraphicsView>

#include "timelineplayhead.h"
#include "timelineviewenditem.h"
#include "widget/timelinewidget/timelinescaledobject.h"

class TimelineViewBase : public QGraphicsView, public TimelineScaledObject
{
  Q_OBJECT
public:
  TimelineViewBase(QWidget* parent = nullptr);

  void SetScale(const double& scale);

  void SetEndTime(const rational& length);

public slots:
  void SetTimebase(const rational& timebase);

  void SetTime(const int64_t time);

signals:
  void TimeChanged(const int64_t& time);

  void ScaleChanged(double scale);

protected:
  virtual void drawForeground(QPainter *painter, const QRectF &rect) override;

  virtual void resizeEvent(QResizeEvent *event) override;

  virtual void ScaleChangedEvent(double scale);

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

  TimelineViewEndItem* end_item_;

  QGraphicsScene scene_;

private slots:
  /**
   * @brief Slot called whenever the view resizes or the scene contents change to enforce minimum scene sizes
   */
  void UpdateSceneRect();

};

#endif // TIMELINEVIEWBASE_H
