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

TimelineWidget::SlideTool::SlideTool(TimelineWidget* parent) :
  PointerTool(parent)
{
  SetTrimmingAllowed(false);
  SetTrackMovementAllowed(false);
}

void TimelineWidget::SlideTool::MouseReleaseInternal(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)

  QUndoCommand* command = new QUndoCommand();

  // Find earliest point to ripple around
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* b = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    if (ghost->mode() == Timeline::kTrimIn) {
      new BlockResizeWithMediaInCommand(b, ghost->AdjustedLength(), command);
    } else if (ghost->mode() == Timeline::kTrimOut) {
      new BlockResizeCommand(b, ghost->AdjustedLength(), command);
    } else if (ghost->mode() == Timeline::kMove && b->previous() == nullptr) {
      GapBlock* gap = new GapBlock();
      gap->set_length_and_media_out(ghost->InAdjustment());
      new NodeAddCommand(static_cast<NodeGraph*>(b->parent()), gap, command);
      new TrackPrependBlockCommand(parent()->GetTrackFromReference(ghost->Track()), gap, command);
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

rational TimelineWidget::SlideTool::FrameValidateInternal(rational time_movement, const QVector<TimelineViewGhostItem *> &ghosts)
{
  // Only validate trimming, and we don't care about "overwriting" since the rolling tool is designed to trim at collisions
  time_movement = ValidateInTrimming(time_movement, ghosts, false);
  time_movement = ValidateOutTrimming(time_movement, ghosts, false);

  return time_movement;
}

void TimelineWidget::SlideTool::InitiateGhosts(TimelineViewBlockItem *clicked_item,
                                               Timeline::MovementMode trim_mode,
                                               bool allow_gap_trimming)
{
  Q_UNUSED(allow_gap_trimming)

  PointerTool::InitiateGhosts(clicked_item, trim_mode, true);

  // For each ghost, we make an equivalent Ghost on the next/previous block
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* ghost_block = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    // Add trimming ghosts for each side of the Block

    if (ghost_block->previous() != nullptr) {
      // Add an extra Ghost for the previous block
      AddGhostFromBlock(ghost_block->previous(), ghost->Track(), Timeline::kTrimOut);
    }

    if (ghost_block->next() != nullptr) {
      AddGhostFromBlock(ghost_block->next(), ghost->Track(), Timeline::kTrimIn);
    }
  }
}

OLIVE_NAMESPACE_EXIT
