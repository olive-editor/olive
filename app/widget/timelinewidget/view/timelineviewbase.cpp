#include "timelineviewbase.h"

#include <QGraphicsRectItem>
#include <QMouseEvent>
#include <QScrollBar>

#include "common/timecodefunctions.h"

TimelineViewBase::TimelineViewBase(QWidget *parent) :
  QGraphicsView(parent),
  playhead_(0),
  playhead_scene_left_(-1),
  playhead_scene_right_(-1)
{
  setScene(&scene_);

  // Create end item
  end_item_ = new TimelineViewEndItem();
  scene_.addItem(end_item_);

  // Set default scale
  SetScale(1.0);

  connect(&scene_, SIGNAL(changed(const QList<QRectF>&)), this, SLOT(UpdateSceneRect()));
}

void TimelineViewBase::SetScale(const double &scale)
{
  scale_ = scale;

  // Force redraw for playhead
  viewport()->update();

  end_item_->SetScale(scale_);

  ScaleChangedEvent(scale_);
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

void TimelineViewBase::SetEndTime(const rational &length)
{
  end_item_->SetEndTime(length);
}

void TimelineViewBase::UpdateSceneRect()
{
  QRectF bounding_rect = scene_.itemsBoundingRect();

  // Ensure the scene height is always AT LEAST the height of the view
  // The scrollbar appears to have a 1px margin on the top and bottom, hence the -2
  int minimum_height = height() - horizontalScrollBar()->height() - 2;

  if (alignment() & Qt::AlignBottom) {
    // Ensure the scene left and bottom are always 0
    bounding_rect.setBottomLeft(QPointF(0, 0));

    if (bounding_rect.top() > minimum_height) {
      bounding_rect.setTop(-minimum_height);
    }
  } else {
    // Ensure the scene left and top are always 0
    bounding_rect.setTopLeft(QPointF(0, 0));

    if (bounding_rect.height() < minimum_height) {
      bounding_rect.setHeight(minimum_height);
    }
  }

  // Ensure the scene is always the full length of the timeline with a gap at the end to work with
  end_item_->SetEndPadding(width()/4);

  // If the scene is already this rect, do nothing
  if (scene_.sceneRect() == bounding_rect) {
    return;
  }

  scene_.setSceneRect(bounding_rect);
}

void TimelineViewBase::resizeEvent(QResizeEvent *event)
{
  QGraphicsView::resizeEvent(event);

  UpdateSceneRect();
}

void TimelineViewBase::ScaleChangedEvent(double scale)
{
  Q_UNUSED(scale)
}
