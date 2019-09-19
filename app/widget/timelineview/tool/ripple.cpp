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

TimelineView::RippleTool::RippleTool(TimelineView* parent) :
  PointerTool(parent)
{
  SetMovementAllowed(false);
}

void TimelineView::RippleTool::MouseReleaseInternal(QMouseEvent *event)
{
  Q_UNUSED(event)

  if (parent()->ghost_items_.isEmpty()) {
    return;
  }

  // Retrieve cursor position difference
  QPointF scene_pos = GetScenePos(event->pos());
  QPointF movement = scene_pos - drag_start_;

  // For ripple operations, all ghosts will be moving the same way
  olive::timeline::MovementMode movement_mode = parent()->ghost_items_.first()->mode();

  // The amount to ripple by
  rational ripple_length = parent()->SceneToTime(movement.x());

  QList<Block *> blocks_to_ripple;

  // Find earliest point to ripple around
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* b = Node::ValueToPtr<Block>(ghost->data(0));

    blocks_to_ripple.append(b);
  }

  emit parent()->RequestRippleBlocks(blocks_to_ripple, ripple_length, movement_mode);
}

rational TimelineView::RippleTool::FrameValidateInternal(rational time_movement, QVector<TimelineViewGhostItem *> ghosts)
{
  // FIXME: Validate rippling

  return PointerTool::FrameValidateInternal(time_movement, ghosts);
}
