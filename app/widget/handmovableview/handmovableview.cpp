/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
  scroll_zooms_by_default_(Config::Current()[QStringLiteral("ScrollZooms")].toBool())
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
                            event->localPos(),
                            Qt::LeftButton,
                            Qt::LeftButton,
                            event->modifiers());

    super::mousePressEvent(&transformed);

    return true;
  }

  return false;
}

bool HandMovableView::HandMove(QMouseEvent *event)
{
  if (dragging_hand_) {
    // Transform mouse event to act like the left button is pressed
    QMouseEvent transformed(event->type(),
                            event->localPos(),
                            Qt::LeftButton,
                            Qt::LeftButton,
                            event->modifiers());

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
                            Qt::LeftButton,
                            Qt::LeftButton,
                            event->modifiers());

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

bool HandMovableView::WheelEventIsAZoomEvent(QWheelEvent *event) const
{
  return (static_cast<bool>(event->modifiers() & Qt::ControlModifier) == !scroll_zooms_by_default_);
}

void HandMovableView::wheelEvent(QWheelEvent *event)
{
  if (WheelEventIsAZoomEvent(event)) {
    if (!event->angleDelta().isNull()) {
      qreal multiplier = 1.0 + (static_cast<qreal>(event->angleDelta().x() + event->angleDelta().y()) * 0.001);

      QPointF cursor_pos;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
      cursor_pos = event->position();
#else
      cursor_pos = event->posF();
#endif

      ZoomIntoCursorPosition(event, multiplier, cursor_pos);
    }
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

QAction *HandMovableView::AddSetScrollZoomsByDefaultActionToMenu(QMenu *m, bool autoconnect)
{
  QAction* ctrl_zoom = m->addAction(tr("Scroll Zooms By Default"));
  ctrl_zoom->setCheckable(true);
  ctrl_zoom->setChecked(GetScrollZoomsByDefault());

  if (autoconnect) {
    connect(ctrl_zoom, &QAction::triggered, this, &HandMovableView::SetScrollZoomsByDefault);
  }

  return ctrl_zoom;
}

}
