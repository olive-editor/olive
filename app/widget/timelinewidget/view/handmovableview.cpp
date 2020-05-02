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

#include "handmovableview.h"

#include <QMouseEvent>

#include "core.h"

OLIVE_NAMESPACE_ENTER

HandMovableView::HandMovableView(QWidget* parent) :
  QGraphicsView(parent),
  dragging_hand_(false)
{
  connect(Core::instance(), &Core::ToolChanged, this, &HandMovableView::ApplicationToolChanged);
}

void HandMovableView::ApplicationToolChanged(Tool::Item tool)
{
  if (tool == Tool::kHand) {
    setDragMode(ScrollHandDrag);
  } else {
    setDragMode(default_drag_mode_);
  }

  ToolChangedEvent(tool);
}

bool HandMovableView::HandPress(QMouseEvent *event)
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

bool HandMovableView::HandMove(QMouseEvent *event)
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

bool HandMovableView::HandRelease(QMouseEvent *event)
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

void HandMovableView::SetDefaultDragMode(QGraphicsView::DragMode mode)
{
  default_drag_mode_ = mode;
  setDragMode(default_drag_mode_);
}

const QGraphicsView::DragMode &HandMovableView::GetDefaultDragMode() const
{
  return default_drag_mode_;
}

OLIVE_NAMESPACE_EXIT
