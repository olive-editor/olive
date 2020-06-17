/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "timelineviewbase.h"

#include <QGraphicsRectItem>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTimer>

#include "common/autoscroll.h"
#include "common/timecodefunctions.h"
#include "config/config.h"

OLIVE_NAMESPACE_ENTER

const double TimelineViewBase::kMaximumScale = 8192;

TimelineViewBase::TimelineViewBase(QWidget *parent) :
  HandMovableView(parent),
  playhead_(0),
  playhead_scene_left_(-1),
  playhead_scene_right_(-1),
  dragging_playhead_(false),
  limit_y_axis_(false),
  snapped_(false),
  snap_service_(nullptr)
{
  setScene(&scene_);

  // Set default scale
  SetScale(1.0);

  SetDefaultDragMode(NoDrag);

  connect(&scene_, SIGNAL(changed(const QList<QRectF>&)), this, SLOT(UpdateSceneRect()));

  SetMaximumScale(kMaximumScale);
}

void TimelineViewBase::TimebaseChangedEvent(const rational &)
{
  // Timebase influences position/visibility of playhead
  viewport()->update();
}

void TimelineViewBase::EnableSnap(const QList<rational> &points)
{
  snapped_ = true;
  snap_time_ = points;

  viewport()->update();
}

void TimelineViewBase::DisableSnap()
{
  snapped_ = false;

  viewport()->update();
}

void TimelineViewBase::SetSnapService(SnapService *service)
{
  snap_service_ = service;
}

void TimelineViewBase::SetTime(const int64_t time)
{
  playhead_ = time;

  switch (static_cast<AutoScroll::Method>(Config::Current()["Autoscroll"].toInt())) {
  case AutoScroll::kNone:
    // Do nothing
    break;
  case AutoScroll::kPage:
    QMetaObject::invokeMethod(this, "PageScrollToPlayhead", Qt::QueuedConnection);
    break;
  case AutoScroll::kSmooth:
    emit RequestCenterScrollOnPlayhead();
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

  if (snapped_) {
    painter->setPen(palette().text().color());

    foreach (const rational& r, snap_time_) {
      double x = TimeToScene(r);

      painter->drawLine(x, rect.top(), x, rect.height());
    }
  }
}

rational TimelineViewBase::GetPlayheadTime() const
{
  return Timecode::timestamp_to_time(playhead_, timebase());
}

bool TimelineViewBase::PlayheadPress(QMouseEvent *event)
{
  QPointF scene_pos = mapToScene(event->pos());

  dragging_playhead_ = (event->button() == Qt::LeftButton
                        && scene_pos.x() >= playhead_scene_left_
                        && scene_pos.x() < playhead_scene_right_);

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

  if (Core::instance()->snapping() && snap_service_) {
    rational target_time = Timecode::timestamp_to_time(target_ts, timebase());
    rational movement;

    snap_service_->SnapPoint({target_time}, &movement, SnapService::kSnapAll & ~SnapService::kSnapToPlayhead);

    if (!movement.isNull()) {
      target_ts = Timecode::time_to_timestamp(target_time + movement, timebase());
    }
  }

  SetTime(target_ts);
  emit TimeChanged(target_ts);

  return true;
}

bool TimelineViewBase::PlayheadRelease(QMouseEvent*)
{
  if (dragging_playhead_) {
    dragging_playhead_ = false;

    if (snap_service_) {
      snap_service_->HideSnaps();
    }

    return true;
  }

  return false;
}

qreal TimelineViewBase::GetPlayheadX()
{
  return TimeToScene(Timecode::timestamp_to_time(playhead_, timebase()));
}

void TimelineViewBase::SetEndTime(const rational &length)
{
  end_time_ = length;

  UpdateSceneRect();
}

void TimelineViewBase::UpdateSceneRect()
{
  QRectF bounding_rect = scene_.itemsBoundingRect();

  // There's no need for a timeline to ever go below 0 on the X scale
  bounding_rect.setLeft(0);

  // Ensure the scene is always the full length of the timeline with a gap at the end to work with
  bounding_rect.setRight(TimeToScene(end_time_) + width() / 2);

  // Any further rect processing from derivatives can be done here
  SceneRectUpdateEvent(bounding_rect);

  // If the scene is already this rect, do nothing
  if (scene_.sceneRect() != bounding_rect) {
    scene_.setSceneRect(bounding_rect);
  }
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

void TimelineViewBase::ScaleChangedEvent(const double &scale)
{
  TimelineScaledObject::ScaleChangedEvent(scale);

  // Update scene rect
  UpdateSceneRect();

  // Force redraw for playhead if the above function didn't do it
  viewport()->update();
}

bool TimelineViewBase::HandleZoomFromScroll(QWheelEvent *event)
{
  if (WheelEventIsAZoomEvent(event)) {
    // If CTRL is held (or a preference is set to swap CTRL behavior), we zoom instead of scrolling
    if (!event->angleDelta().isNull()) {
      if (event->angleDelta().x() + event->angleDelta().y() > 0) {
        emit ScaleChanged(GetScale() * 1.1);
      } else {
        emit ScaleChanged(GetScale() * 0.9);
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

void TimelineViewBase::SetLimitYAxis(bool)
{
  limit_y_axis_ = true;
  UpdateSceneRect();
}

OLIVE_NAMESPACE_EXIT
