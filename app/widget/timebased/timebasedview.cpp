/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "timebasedview.h"

#include <QGraphicsRectItem>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTimer>

#include "widget/timebased/timebasedwidget.h"

namespace olive {

TimeBasedView::TimeBasedView(QWidget *parent) :
  HandMovableView(parent),
  playhead_scene_left_(-1),
  playhead_scene_right_(-1),
  dragging_playhead_(false),
  snapped_(false),
  snap_service_(nullptr),
  y_axis_enabled_(false),
  y_scale_(1.0),
  viewer_(nullptr)
{
  // Sets scene to our scene
  setScene(&scene_);

  // Set default scale (ensures non-zero scale from beginning)
  SetScale(1.0);

  // Default to no default drag mode
  SetDefaultDragMode(NoDrag);

  // Signal to update bounding rect when the scene changes
  connect(&scene_, &QGraphicsScene::changed, this, &TimeBasedView::UpdateSceneRect);

  // Workaround for Qt drawing issues with the default MinimalViewportUpdate. While this might be
  // slower (Qt documentation says it may actually be faster in some situations),
  // MinimalViewportUpdate causes all sorts of graphical crud building up in the scene
  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
}

void TimeBasedView::TimebaseChangedEvent(const rational &)
{
  // Timebase influences position/visibility of playhead
  viewport()->update();
}

void TimeBasedView::EnableSnap(const std::vector<rational> &points)
{
  snapped_ = true;
  snap_time_ = points;

  viewport()->update();
}

void TimeBasedView::DisableSnap()
{
  snapped_ = false;

  viewport()->update();
}

const double &TimeBasedView::GetYScale() const
{
  return y_scale_;
}

void TimeBasedView::VerticalScaleChangedEvent(double)
{
}

void TimeBasedView::ZoomIntoCursorPosition(QWheelEvent *event, double scale_multiplier, const QPointF &cursor_pos)
{
  // If CTRL is held (or a preference is set to swap CTRL behavior), we zoom instead of scrolling
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

  if (!only_vertical) {
    double old_scroll = horizontalScrollBar()->value();

    double old_scale = GetScale();
    emit ScaleChanged(old_scale * scale_multiplier);

    // Use GetScale so that if this value was clamped, we don't erroneously use an unclamped value
    int new_x_scroll = qRound((cursor_pos.x() + old_scroll) / old_scale * GetScale() - cursor_pos.x());
    horizontalScrollBar()->setValue(new_x_scroll);
  }

  if (!only_horizontal) {
    double old_y_scroll = verticalScrollBar()->value();

    double old_y_scale = GetYScale();
    SetYScale(old_y_scale * scale_multiplier);

    // Use GetYScale so that if this value was clamped, we don't erroneously use an unclamped value
    int new_y_scroll = qRound((cursor_pos.y() + old_y_scroll) / old_y_scale * GetYScale() - cursor_pos.y());
    verticalScrollBar()->setValue(new_y_scroll);
  }
}

void TimeBasedView::SetYScale(const double &y_scale)
{
  Q_ASSERT(y_scale > 0);

  y_scale_ = y_scale;

  if (y_axis_enabled_) {
    VerticalScaleChangedEvent(y_scale_);

    viewport()->update();
  }
}

void TimeBasedView::SetViewerNode(ViewerOutput *v)
{
  if (viewer_) {
    disconnect(viewer_, &ViewerOutput::PlayheadChanged, viewport(), static_cast<void(QWidget::*)()>(&TimeBasedView::update));
  }

  viewer_ = v;

  if (viewer_) {
    connect(viewer_, &ViewerOutput::PlayheadChanged, viewport(), static_cast<void(QWidget::*)()>(&TimeBasedView::update));
  }
}

QPointF TimeBasedView::ScalePoint(const QPointF &p) const
{
  return QPointF(p.x() * GetScale(), p.y() * GetYScale());
}

QPointF TimeBasedView::UnscalePoint(const QPointF &p) const
{
  return QPointF(p.x() / GetScale(), p.y() / GetYScale());
}

void TimeBasedView::drawForeground(QPainter *painter, const QRectF &rect)
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

bool TimeBasedView::PlayheadPress(QMouseEvent *event)
{
  QPointF scene_pos = mapToScene(event->pos());

  dragging_playhead_ = (event->button() == Qt::LeftButton
                        && scene_pos.x() >= playhead_scene_left_
                        && scene_pos.x() < playhead_scene_right_);

  return dragging_playhead_;
}

bool TimeBasedView::PlayheadMove(QMouseEvent *event)
{
  if (!dragging_playhead_) {
    return false;
  }

  if (viewer_) {
    QPointF scene_pos = mapToScene(event->pos());
    rational mouse_time = qMax(rational(0), SceneToTime(scene_pos.x()));

    if (Core::instance()->snapping() && snap_service_) {
      rational movement;

      snap_service_->SnapPoint({mouse_time}, &movement, TimeBasedWidget::kSnapAll & ~TimeBasedWidget::kSnapToPlayhead);

      mouse_time += movement;
    }

    viewer_->SetPlayhead(mouse_time);
  }

  return true;
}

bool TimeBasedView::PlayheadRelease(QMouseEvent*)
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

qreal TimeBasedView::GetPlayheadX()
{
  if (viewer_) {
    return TimeToScene(viewer_->GetPlayhead());
  } else {
    return 0;
  }
}

void TimeBasedView::SetEndTime(const rational &length)
{
  end_time_ = length;

  UpdateSceneRect();
}

void TimeBasedView::UpdateSceneRect()
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

void TimeBasedView::resizeEvent(QResizeEvent *event)
{
  QGraphicsView::resizeEvent(event);

  UpdateSceneRect();
}

void TimeBasedView::ScaleChangedEvent(const double &scale)
{
  TimeScaledObject::ScaleChangedEvent(scale);

  // Update scene rect
  UpdateSceneRect();

  // Force redraw for playhead if the above function didn't do it
  viewport()->update();
}

}
