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

#include "widget/timelinewidget/timelinewidget.h"

#include <QToolTip>

#include "common/timecodefunctions.h"
#include "config/config.h"
#include "slip.h"

namespace olive {

SlipTool::SlipTool(TimelineWidget *parent) :
  PointerTool(parent)
{
  SetTrimmingAllowed(false);
  SetTrackMovementAllowed(false);
}

void SlipTool::ProcessDrag(const TimelineCoordinate &mouse_pos)
{
  // Determine frame movement
  rational time_movement = drag_start_.GetFrame() - mouse_pos.GetFrame();

  // Validate slip (enforce all ghosts moving in legal ways)
  foreach (TimelineViewGhostItem* ghost, parent()->GetGhostItems()) {
    if (ghost->GetMediaIn() + time_movement < 0) {
      time_movement = -ghost->GetMediaIn();
    }
  }

  // Perform slip
  foreach (TimelineViewGhostItem* ghost, parent()->GetGhostItems()) {
    ghost->SetMediaInAdjustment(time_movement);
  }

  // Generate tooltip and force it to to update (otherwise the tooltip won't move as written in the
  // documentation, and could get in the way of the cursor)
  rational tooltip_timebase = parent()->GetTimebaseForTrackType(drag_start_.GetTrack().type());
  QToolTip::hideText();
  QToolTip::showText(QCursor::pos(),
                     Timecode::timestamp_to_timecode(Timecode::time_to_timestamp(time_movement, tooltip_timebase),
                                                                              tooltip_timebase,
                                                                              Core::instance()->GetTimecodeDisplay(),
                                                                              true),
                     parent());
}

void SlipTool::FinishDrag(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)

  MultiUndoCommand* command = new MultiUndoCommand();

  // Find earliest point to ripple around
  foreach (TimelineViewGhostItem* ghost, parent()->GetGhostItems()) {
    Block* b = Node::ValueToPtr<Block>(ghost->GetData(TimelineViewGhostItem::kAttachedBlock));

    command->add_child(new BlockSetMediaInCommand(b, ghost->GetAdjustedMediaIn()));
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

}
