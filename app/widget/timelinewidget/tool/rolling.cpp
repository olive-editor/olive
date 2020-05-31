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

#include "node/block/gap/gap.h"
#include "widget/nodeview/nodeviewundo.h"

OLIVE_NAMESPACE_ENTER

TimelineWidget::RollingTool::RollingTool(TimelineWidget* parent) :
  PointerTool(parent)
{
  SetMovementAllowed(false);
  SetTrimOverwriteAllowed(true);
}

void TimelineWidget::RollingTool::InitiateDrag(TimelineViewBlockItem *clicked_item,
                                               Timeline::MovementMode trim_mode,
                                               bool allow_gap_trimming)
{
  PointerTool::InitiateDrag(clicked_item, trim_mode, true);

  // For each ghost, we make an equivalent Ghost on the next/previous block
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* ghost_block = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    if (ghost->mode() == Timeline::kTrimIn && ghost_block->previous()) {
      // Add an extra Ghost for the previous block
      AddGhostFromBlock(ghost_block->previous(), ghost->Track(), Timeline::kTrimOut);
    } else if (ghost->mode() == Timeline::kTrimOut && ghost_block->next()) {
      AddGhostFromBlock(ghost_block->next(), ghost->Track(), Timeline::kTrimIn);
    }
  }
}

void TimelineWidget::RollingTool::FinishDrag(TimelineViewMouseEvent *event)
{
  QUndoCommand* command = new QUndoCommand();

  // Find earliest point to ripple around
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    if (ghost->mode() == drag_movement_mode()) {
      Block* b = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

      BlockTrimCommand* c = new BlockTrimCommand(parent()->GetTrackFromReference(ghost->Track()),
                                                 b,
                                                 ghost->AdjustedLength(),
                                                 drag_movement_mode(),
                                                 command);
      c->SetAllowNonGapTrimming(true);
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

OLIVE_NAMESPACE_EXIT
