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

  // Retrieve cursor position difference
  QPointF scene_pos = GetScenePos(event->pos());
  QPointF movement = scene_pos - drag_start_;

  // The point to ripple all clips after (we use the earliest point possible)
  rational ripple_point = RATIONAL_MAX;

  // The amount to ripple by
  rational ripple_length = parent()->SceneToTime(movement.x());

  // Find earliest point to ripple around
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    if (ghost->mode() == TimelineViewGhostItem::kTrimIn) {
      ripple_point = qMin(ripple_point, ghost->In());
    } else if (ghost->mode() == TimelineViewGhostItem::kTrimOut) {
      ripple_point = qMin(ripple_point, ghost->Out());
    }
  }


}

rational TimelineView::RippleTool::FrameValidateInternal(rational time_movement, QVector<TimelineViewGhostItem *> ghosts)
{
  // FIXME: Validate rippling

  return PointerTool::FrameValidateInternal(time_movement, ghosts);
}
