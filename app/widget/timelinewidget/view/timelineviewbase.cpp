/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

namespace olive {

const double TimelineViewBase::kMaximumScale = 8192;

TimelineViewBase::TimelineViewBase(QWidget *parent) :
  HandMovableView(parent),
  playhead_(0),
  playhead_scene_left_(-1),
  playhead_scene_right_(-1),
  dragging_playhead_(false),
  snapped_(false),
  snap_service_(nullptr),
  y_axis_enabled_(false),
  y_scale_(1.0)
{
  // Sets scene to our scene
  setScene(&scene_);

  // Set default scale (ensures non-zero scale from beginning)
  SetScale(1.0);

  // Default to no default drag mode
  SetDefaultDragMode(NoDrag);

  // Signal to update bounding rect when the scene changes
  connect(&scene_, &QGraphicsScene::changed, this, &TimelineViewBase::UpdateSceneRect);

  // Always enforce maximum scale
  SetMaximumScale(kMaximumScale);

  // Workaround for Qt drawing issues with the default MinimalViewportUpdate. While this might be
  // slower (Qt documentation says it may actually be faster in some situations),
  // MinimalViewportUpdate causes all sorts of graphical crud building up in the scene
  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
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

const double &TimelineViewBase::GetYScale() const
{
  return y_scale_;
}

void TimelineViewBase::VerticalScaleChangedEvent(double)
{
}

void TimelineViewBase::SetYScale(const double &y_scale)
{
  y_scale_ = y_scale;

  if (y_axis_enabled_) {
    VerticalScaleChangedEvent(y_scale_);

    viewport()->update();
  }
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

    QRectF playhead_rect(playhead_scene_left_, rect.top(), width, rect.height());

    // Get playhead highlight color
    QColor highlight = palette().text().color();
    highlight.setAlpha(32);
    painter->setPen(Qt::NoPen);
    painter->setBrush(highlight);
    painter->drawRect(playhead_rect);

    // FIXME: Hardcoded...
    painter->setPen(PLAYHEAD_COLOR);
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(QLineF(playhead_rect.topLeft(), playhead_rect.bottomLeft()));
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
  bounding_rect.setRight(TimeToScene(end_time_) + width());

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
      bool only_vertical = false;
      bool only_horizontal = false;

      // Ctrl+Shift limits to only one axis
      // Alt switches between horizontal only (alt held) or vertical only (alt not held)
      if (y_axis_enabled_) {
        if (event->modifiers() & Qt::ShiftModifier) {
          if (event->modifiers() & Qt::AltModifier) {
            only_horizontal = true;
          } else {
            only_vertical = true;
          }
        }
      } else {
        only_horizontal = true;
      }

      double scale_multiplier;

      if (event->angleDelta().x() + event->angleDelta().y() > 0) {
        scale_multiplier = 1.1;
      } else {
        scale_multiplier = 0.9;
      }

      QPointF cursor_pos;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
      cursor_pos = event->position();
#else
      cursor_pos = event->posF();
#endif

      if (!only_vertical) {
        double new_x_scale = GetScale() * scale_multiplier;

        int new_x_scroll = qRound(horizontalScrollBar()->value() / GetScale() * new_x_scale + (cursor_pos.x() - cursor_pos.x() / new_x_scale * GetScale()));

        emit ScaleChanged(new_x_scale);

        horizontalScrollBar()->setValue(new_x_scroll);
      }

      if (!only_horizontal) {
        double new_y_scale = GetYScale() * scale_multiplier;

        int new_y_scroll = qRound(verticalScrollBar()->value() / GetYScale() * new_y_scale + (cursor_pos.y() - cursor_pos.y() / new_y_scale * GetYScale()));

        SetYScale(new_y_scale);

        verticalScrollBar()->setValue(new_y_scroll);
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

}
