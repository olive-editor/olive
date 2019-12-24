#include "timelineviewbase.h"

#include <QGraphicsRectItem>
#include <QMouseEvent>

#include "common/timecodefunctions.h"

TimelineViewBase::TimelineViewBase(QWidget *parent) :
  QGraphicsView(parent),
  playhead_(0),
  playhead_scene_left_(-1),
  playhead_scene_right_(-1)
{
}

void TimelineViewBase::SetTimebase(const rational &timebase)
{
  SetTimebaseInternal(timebase);

  // Timebase influences position/visibility of playhead
  viewport()->update();
}

void TimelineViewBase::SetTime(const int64_t time)
{
  playhead_ = time;

  // Force redraw for playhead
  viewport()->update();
}

void TimelineViewBase::drawForeground(QPainter *painter, const QRectF &rect)
{
  QGraphicsView::drawForeground(painter, rect);

  if (!timebase().isNull()) {
    double width = TimeToScene(timebase());

    playhead_scene_left_ = TimeToScene(rational(playhead_ * timebase().numerator(), timebase().denominator()));
    playhead_scene_right_ = playhead_scene_left_ + width;

    playhead_style_.Draw(painter, QRectF(playhead_scene_left_, rect.top(), width, rect.height()));
  }
}

rational TimelineViewBase::GetPlayheadTime()
{
  return rational(playhead_ * timebase().numerator(), timebase().denominator());
}

bool TimelineViewBase::PlayheadPress(QMouseEvent *event)
{
  QPointF scene_pos = mapToScene(event->pos());

  dragging_playhead_ = (scene_pos.x() >= playhead_scene_left_ && scene_pos.x() < playhead_scene_right_);

  return dragging_playhead_;
}

bool TimelineViewBase::PlayheadMove(QMouseEvent *event)
{
  if (!dragging_playhead_) {
    return false;
  }

  QPointF scene_pos = mapToScene(event->pos());
  rational mouse_time = SceneToTime(scene_pos.x());

  int64_t target_ts = olive::time_to_timestamp(mouse_time, timebase());

  SetTime(target_ts);
  emit TimeChanged(target_ts);

  return true;
}

bool TimelineViewBase::PlayheadRelease(QMouseEvent *event)
{
  return dragging_playhead_;
}
