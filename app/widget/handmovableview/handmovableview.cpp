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

#include "handmovableview.h"

#include <QMouseEvent>

#include "config/config.h"
#include "core.h"

namespace olive {

#define super QGraphicsView

HandMovableView::HandMovableView(QWidget* parent) :
  super(parent),
  dragging_hand_(false),
  is_timeline_axes_(false)
{
  connect(Core::instance(), &Core::ToolChanged, this, &HandMovableView::ApplicationToolChanged);
}

void HandMovableView::ApplicationToolChanged(Tool::Item tool)
{
  if (tool == Tool::kHand) {
    setDragMode(ScrollHandDrag);
    setInteractive(false);
  } else {
    setDragMode(default_drag_mode_);
    setInteractive(true);
  }

  ToolChangedEvent(tool);
}

bool HandMovableView::HandPress(QMouseEvent *event)
{
  if (event->button() == Qt::MiddleButton) {
    pre_hand_drag_mode_ = dragMode();
    dragging_hand_ = true;

    setDragMode(ScrollHandDrag);
    setInteractive(false);

    // Transform mouse event to act like the left button is pressed
    QMouseEvent transformed(event->type(),
                            event->pos(),
                            Qt::LeftButton,
                            Qt::LeftButton,
                            event->modifiers());

    transformed_pos_ = QPoint(0, 0);

    super::mousePressEvent(&transformed);

    return true;
  }

  return false;
}

bool HandMovableView::HandMove(QMouseEvent *event)
{
  if (dragging_hand_) {
    // Transform mouse event to act like the left button is pressed
    QPoint adjustment(0, 0);

    QMouseEvent transformed(event->type(),
                            event->pos() - transformed_pos_,
                            Qt::LeftButton,
                            Qt::LeftButton,
                            event->modifiers());

    if (event->pos().x() < 0) {
      transformed_pos_.setX(transformed_pos_.x() + width());
      adjustment.setX(width());
    } else if (event->pos().x() >= width()) {
      transformed_pos_.setX(transformed_pos_.x() - width());
      adjustment.setX(-width());
    }

    if (event->pos().y() < 0) {
      transformed_pos_.setY(transformed_pos_.y() + height());
      adjustment.setY(height());
    } else if (event->pos().y() >= height()) {
      transformed_pos_.setY(transformed_pos_.y() - height());
      adjustment.setY(-height());
    }

    if (!adjustment.isNull()) {
      QCursor::setPos(QCursor::pos() + adjustment);
    }

    super::mouseMoveEvent(&transformed);
  }
  return dragging_hand_;
}

bool HandMovableView::HandRelease(QMouseEvent *event)
{
  if (dragging_hand_) {
    // Transform mouse event to act like the left button is pressed
    QMouseEvent transformed(event->type(),
                            event->localPos(),
                            event->windowPos(),
                            event->screenPos(),
                            Qt::LeftButton,
                            Qt::LeftButton,
                            event->modifiers(),
                            event->source());

    super::mouseReleaseEvent(&transformed);

    setInteractive(true);
    setDragMode(pre_hand_drag_mode_);

    dragging_hand_ = false;

    return true;
  }

  return false;
}

void HandMovableView::SetDefaultDragMode(HandMovableView::DragMode mode)
{
  default_drag_mode_ = mode;
  setDragMode(default_drag_mode_);
}

const HandMovableView::DragMode &HandMovableView::GetDefaultDragMode() const
{
  return default_drag_mode_;
}

bool HandMovableView::WheelEventIsAZoomEvent(QWheelEvent *event)
{
  return (static_cast<bool>(event->modifiers() & Qt::ControlModifier) == !OLIVE_CONFIG("ScrollZooms").toBool());
}

qreal HandMovableView::GetScrollZoomMultiplier(QWheelEvent *event)
{
  qreal v = (static_cast<qreal>(event->angleDelta().x() + event->angleDelta().y()) * 0.001);
  if (event->inverted()) {
    v = -v;
  }
  return 1.0 + v;
}

void HandMovableView::wheelEvent(QWheelEvent *event)
{
  if (WheelEventIsAZoomEvent(event)) {
    if (!event->angleDelta().isNull()) {
      qreal multiplier = GetScrollZoomMultiplier(event);

      QPointF cursor_pos;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
      cursor_pos = event->position();
#else
      cursor_pos = event->posF();
#endif

      ZoomIntoCursorPosition(event, multiplier, cursor_pos);
    }
  } else if (is_timeline_axes_) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))

    QPoint angle_delta = event->angleDelta();

    if (OLIVE_CONFIG("InvertTimelineScrollAxes").toBool() // Check if config is set to invert timeline axes
        && event->source() != Qt::MouseEventSynthesizedBySystem) { // Never flip axes on Apple trackpads though
      angle_delta = QPoint(angle_delta.y(), angle_delta.x());
    }

    QWheelEvent e(
      #if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
          event->position(),
          event->globalPosition(),
      #else
          event->pos(),
          event->globalPos(),
      #endif
          event->pixelDelta(),
          angle_delta,
          event->buttons(),
          event->modifiers(),
          event->phase(),
          event->inverted(),
          event->source()
          );

#else

    Qt::Orientation orientation = event->orientation();

    if (OLIVE_CONFIG("InvertTimelineScrollAxes").toBool()) {
      orientation = (orientation == Qt::Horizontal) ? Qt::Vertical : Qt::Horizontal;
    }

    QWheelEvent e(
          event->pos(),
          event->globalPos(),
          event->pixelDelta(),
          event->angleDelta(),
          event->delta(),
          orientation,
          event->buttons(),
          event->modifiers()
          );
#endif

    super::wheelEvent(&e);
  } else {
    super::wheelEvent(event);
  }
}

void HandMovableView::ZoomIntoCursorPosition(QWheelEvent *event, double multiplier, const QPointF &cursor_pos)
{
  Q_UNUSED(event)
  Q_UNUSED(multiplier)
  Q_UNUSED(cursor_pos)
}

}
