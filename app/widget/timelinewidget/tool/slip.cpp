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

#include "widget/timelinewidget/timelinewidget.h"

#include <QToolTip>

#include "common/timecodefunctions.h"
#include "config/config.h"

TimelineWidget::SlipTool::SlipTool(TimelineWidget *parent) :
  PointerTool(parent)
{
  SetTrimmingAllowed(false);
  SetTrackMovementAllowed(false);
}

void TimelineWidget::SlipTool::ProcessDrag(const TimelineCoordinate &mouse_pos)
{
  // Determine frame movement
  rational time_movement = drag_start_.GetFrame() - mouse_pos.GetFrame();

  // Validate slip (enforce all ghosts moving in legal ways)
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    if (ghost->MediaIn() + time_movement < 0) {
      time_movement = -ghost->MediaIn();
    }
  }

  // Perform slip
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    ghost->SetMediaInAdjustment(time_movement);
  }

  // Show tooltip
  // Generate tooltip (showing earliest in point of imported clip)
  int64_t earliest_timestamp = Timecode::time_to_timestamp(time_movement, parent()->timebase());
  QString tooltip_text = Timecode::timestamp_to_timecode(earliest_timestamp,
                                                         parent()->timebase(),
                                                         Timecode::CurrentDisplay(),
                                                         true);
  // Force tooltip to update (otherwise the tooltip won't move as written in the documentation, and could get in the way
  // of the cursor)
  QToolTip::hideText();
  QToolTip::showText(QCursor::pos(),
                     tooltip_text,
                     parent());
}

void TimelineWidget::SlipTool::MouseReleaseInternal(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)

  QUndoCommand* command = new QUndoCommand();

  // Find earliest point to ripple around
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* b = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    new BlockSetMediaInCommand(b, ghost->GetAdjustedMediaIn(), command);
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

