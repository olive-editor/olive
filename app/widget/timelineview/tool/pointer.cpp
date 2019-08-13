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

#include "widget/timelineview/timelineview.h"

TimelineView::PointerTool::PointerTool(TimelineView *parent) :
  Tool(parent)
{
}

void FlipControlAndShiftModifiers(QMouseEvent* e) {
  if (e->modifiers() & Qt::ControlModifier & Qt::ShiftModifier) {
    return;
  }

  if (e->modifiers() & Qt::ShiftModifier) {
    e->setModifiers((e->modifiers() | Qt::ControlModifier) & ~Qt::ShiftModifier);
  } else if (e->modifiers() & Qt::ControlModifier) {
    e->setModifiers((e->modifiers() | Qt::ShiftModifier) & ~Qt::ControlModifier);
  }
}

void TimelineView::PointerTool::MousePress(QMouseEvent *event)
{
  FlipControlAndShiftModifiers(event);

  parent()->QGraphicsView::mousePressEvent(event);
}

void TimelineView::PointerTool::MouseMove(QMouseEvent *event)
{
  FlipControlAndShiftModifiers(event);

  parent()->QGraphicsView::mouseMoveEvent(event);

  if (!dragging_) {

    // Let's see if there's anything selected to drag
    if (parent()->itemAt(event->pos()) != nullptr) {
      QList<QGraphicsItem*> selected_items = parent()->scene_.selectedItems();

      foreach (QGraphicsItem* item, selected_items) {
        TimelineViewGhostItem* ghost = new TimelineViewGhostItem();
        TimelineViewClipItem* clip_item = static_cast<TimelineViewClipItem*>(item);

        ClipBlock* clip = clip_item->clip();

        ghost->SetIn(clip->in());
        ghost->SetOut(clip->out());
        ghost->SetScale(parent()->scale_);

        ghost->setPos(clip_item->pos());

        parent()->ghost_items_.append(ghost);

        parent()->scene_.addItem(ghost);
      }

      drag_start_ = event->pos();
    }

    dragging_ = true;

  } else if (!parent()->ghost_items_.isEmpty()) {
    QPoint movement = event->pos() - drag_start_;

    rational time_movement = parent()->ScreenToTime(movement.x());

    // Validate movement
    foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
      rational validator = ghost->In() + time_movement;
      if (validator < 0) {
        time_movement = rational(0) - ghost->In();
      }
    }

    // Perform movement
    foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
      ghost->SetInAdjustment(time_movement);
      ghost->SetOutAdjustment(time_movement);
    }
  }
}

void TimelineView::PointerTool::MouseRelease(QMouseEvent *event)
{
  FlipControlAndShiftModifiers(event);

  parent()->QGraphicsView::mouseReleaseEvent(event);

  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* b = Node::ValueToPtr<Block>(ghost->data());

    emit parent()->RequestPlaceBlock(b, ghost->In());
  }

  parent()->ClearGhosts();

  dragging_ = false;
}
