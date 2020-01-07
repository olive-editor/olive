#include "timelineviewbase.h"

#include <QGraphicsRectItem>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTimer>

#include "common/autoscroll.h"
#include "common/timecodefunctions.h"
#include "config/config.h"

const double TimelineViewBase::kMaximumScale = 8192;

TimelineViewBase::TimelineViewBase(QWidget *parent) :
  QGraphicsView(parent),
  playhead_(0),
  playhead_scene_left_(-1),
  playhead_scene_right_(-1),
  dragging_playhead_(false),
  dragging_hand_(false),
  limit_y_axis_(false)
{
  setScene(&scene_);

  // Create end item
  end_item_ = new TimelineViewEndItem();
  scene_.addItem(end_item_);

  // Set default scale
  SetScale(1.0, true);

  SetDefaultDragMode(NoDrag);

  setViewportUpdateMode(FullViewportUpdate);

  connect(&scene_, SIGNAL(changed(const QList<QRectF>&)), this, SLOT(UpdateSceneRect()));
  connect(Core::instance(), &Core::ToolChanged, this, &TimelineViewBase::ApplicationToolChanged);
}

void TimelineViewBase::SetScale(const double &scale, bool center_on_playhead)
{
  scale_ = qMin(scale, kMaximumScale);

  end_item_->SetScale(scale_);

  ScaleChangedEvent(scale_);

  // Force redraw for playhead
  viewport()->update();

  if (center_on_playhead) {
    // Zoom towards the playhead
    // (using a hacky singleShot so the scroll occurs after the scene and its scrollbars have updated)
    QTimer::singleShot(0, this, &TimelineViewBase::CenterScrollOnPlayhead);
  }
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

  switch (static_cast<AutoScroll::Method>(Config::Current()["Autoscroll"].toInt())) {
  case AutoScroll::kNone:
    // Do nothing
    break;
  case AutoScroll::kPage:
    PageScrollToPlayhead();
    break;
  case AutoScroll::kSmooth:
    CenterScrollOnPlayhead();
    break;
  }

  // Force redraw for playhead
  viewport()->update();
}

void TimelineViewBase::drawForeground(QPainter *painter, const QRectF &rect)
{
  QGraphicsView::drawForeground(painter, rect);

  if (!timebase().isNull()) {
    double width = TimeToScene(timebase());

    playhead_scene_left_ = GetPlayheadX();
    playhead_scene_right_ = playhead_scene_left_ + width;

    playhead_style_.Draw(painter, QRectF(playhead_scene_left_, rect.top(), width, rect.height()));
  }
}

rational TimelineViewBase::GetPlayheadTime()
{
  return rational(playhead_ * timebase().numerator(), timebase().denominator());
}

void TimelineViewBase::SetDefaultDragMode(QGraphicsView::DragMode mode)
{
  default_drag_mode_ = mode;
  setDragMode(default_drag_mode_);
}

const QGraphicsView::DragMode &TimelineViewBase::GetDefaultDragMode() const
{
  return default_drag_mode_;
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

  int64_t target_ts = qMax(static_cast<int64_t>(0), Timecode::time_to_timestamp(mouse_time, timebase()));

  SetTime(target_ts);
  emit TimeChanged(target_ts);

  return true;
}

bool TimelineViewBase::PlayheadRelease(QMouseEvent *event)
{
  if (dragging_playhead_) {
    dragging_playhead_ = false;

    return true;
  }

  return false;
}

bool TimelineViewBase::HandPress(QMouseEvent *event)
{
  if (event->button() == Qt::MiddleButton) {
    pre_hand_drag_mode_ = dragMode();
    dragging_hand_ = true;

    setDragMode(ScrollHandDrag);

    // Transform mouse event to act like the left button is pressed
    QMouseEvent transformed(event->type(),
                            event->localPos(),
                            Qt::LeftButton,
                            Qt::LeftButton,
                            event->modifiers());

    QGraphicsView::mousePressEvent(&transformed);

    return true;
  }

  return false;
}

bool TimelineViewBase::HandMove(QMouseEvent *event)
{
  if (dragging_hand_) {
    // Transform mouse event to act like the left button is pressed
    QMouseEvent transformed(event->type(),
                            event->localPos(),
                            Qt::LeftButton,
                            Qt::LeftButton,
                            event->modifiers());

    QGraphicsView::mouseMoveEvent(&transformed);
  }
  return dragging_hand_;
}

bool TimelineViewBase::HandRelease(QMouseEvent *event)
{
  if (dragging_hand_) {
    // Transform mouse event to act like the left button is pressed
    QMouseEvent transformed(event->type(),
                            event->localPos(),
                            Qt::LeftButton,
                            Qt::LeftButton,
                            event->modifiers());

    QGraphicsView::mouseReleaseEvent(&transformed);

    setDragMode(pre_hand_drag_mode_);

    dragging_hand_ = false;

    return true;
  }

  return false;
}

void TimelineViewBase::ToolChangedEvent(Tool::Item tool)
{
}

qreal TimelineViewBase::GetPlayheadX()
{
  return TimeToScene(rational(playhead_ * timebase().numerator(), timebase().denominator()));
}

void TimelineViewBase::SetEndTime(const rational &length)
{
  end_item_->SetEndTime(length);
}

void TimelineViewBase::UpdateSceneRect()
{
  QRectF bounding_rect = scene_.itemsBoundingRect();

  if (limit_y_axis_) {
    // Make a gap of half the viewport height
    if (alignment() & Qt::AlignBottom) {
      bounding_rect.setTop(bounding_rect.top() - height()/2);
    } else {
      bounding_rect.setBottom(bounding_rect.bottom() + height()/2);
    }

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
  } else {
    // We'll still limit the X to 0 since that behavior is desired by all TimelineViewBase derivatives
    bounding_rect.setLeft(0);
  }

  // Ensure the scene is always the full length of the timeline with a gap at the end to work with
  end_item_->SetEndPadding(width()/2);

  // If the scene is already this rect, do nothing
  if (scene_.sceneRect() == bounding_rect) {
    return;
  }

  scene_.setSceneRect(bounding_rect);
}

void TimelineViewBase::CenterScrollOnPlayhead()
{
  horizontalScrollBar()->setValue(qRound(GetPlayheadX()) - viewport()->width()/2);
}

void TimelineViewBase::PageScrollToPlayhead()
{
  int playhead_pos = qRound(GetPlayheadX());

  int viewport_padding = viewport()->width() / 16;

  if (playhead_pos < horizontalScrollBar()->value()) {
    // Anchor the playhead to the RIGHT of where we scroll to
    horizontalScrollBar()->setValue(playhead_pos - viewport()->width() + viewport_padding);
  } else if (playhead_pos > horizontalScrollBar()->value() + viewport()->width()) {
    // Anchor the playhead to the LEFT of where we scroll to
    horizontalScrollBar()->setValue(playhead_pos - viewport_padding);
  }
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

bool TimelineViewBase::HandleZoomFromScroll(QWheelEvent *event)
{
  if (WheelEventIsAZoomEvent(event)) {
    // If CTRL is held (or a preference is set to swap CTRL behavior), we zoom instead of scrolling
    if (event->delta() != 0) {
      if (event->delta() > 0) {
        emit ScaleChanged(scale_ * 2.0);
      } else {
        emit ScaleChanged(scale_ * 0.5);
      }
    }

    return true;
  }

  return false;
}

bool TimelineViewBase::WheelEventIsAZoomEvent(QWheelEvent *event)
{
  return (static_cast<bool>(event->modifiers() & Qt::ControlModifier) == !Config::Current()["ScrollZooms"].toBool());
}

void TimelineViewBase::SetLimitYAxis(bool e)
{
  limit_y_axis_ = true;
  UpdateSceneRect();
}

void TimelineViewBase::ApplicationToolChanged(Tool::Item tool)
{
  if (tool == Tool::kHand) {
    setDragMode(ScrollHandDrag);
  } else {
    setDragMode(default_drag_mode_);
  }

  ToolChangedEvent(tool);
}
