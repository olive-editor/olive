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

TimelineWidget::RippleTool::RippleTool(TimelineWidget* parent) :
  PointerTool(parent)
{
  SetMovementAllowed(false);
}

void TimelineWidget::RippleTool::MouseReleaseInternal(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)

  // For ripple operations, all ghosts will be moving the same way
  Timeline::MovementMode movement_mode = parent()->ghost_items_.first()->mode();

  QUndoCommand* command = new QUndoCommand();

  // Find earliest point to ripple around
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* b = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    if (b == nullptr) {
      // This is a gap we are creating

      // Make sure there's actually a gap being created
      if (ghost->AdjustedLength() > 0) {
        GapBlock* gap = new GapBlock();
        gap->set_length_and_media_out(ghost->AdjustedLength());
        new NodeAddCommand(static_cast<NodeGraph*>(parent()->timeline_node_->parent()), gap, command);

        Block* block_to_append_gap_to = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kReferenceBlock));

        new TrackInsertBlockBetweenBlocksCommand(parent()->GetTrackFromReference(ghost->Track()),
                                                 gap,
                                                 block_to_append_gap_to,
                                                 block_to_append_gap_to->next(),
                                                 command);
      }
    } else {
      // This was a Block that already existed
      if (ghost->AdjustedLength() > 0) {
        if (movement_mode == Timeline::kTrimIn) {
          // We'll need to shift the media in point too
          new BlockResizeWithMediaInCommand(b, ghost->AdjustedLength(), command);
        } else {
          new BlockResizeCommand(b, ghost->AdjustedLength(), command);
        }
      } else {
        // Assumed the Block was a Gap and it was reduced to zero length, remove it here
        new TrackRippleRemoveBlockCommand(parent()->GetTrackFromReference(ghost->Track()), b, command);

        new NodeRemoveWithExclusiveDeps(static_cast<NodeGraph*>(b->parent()), b, command);
      }
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

rational TimelineWidget::RippleTool::FrameValidateInternal(rational time_movement, const QVector<TimelineViewGhostItem *> &ghosts)
{
  // Only validate trimming, and we don't care about "overwriting" since the ripple tool is nondestructive
  time_movement = ValidateInTrimming(time_movement, ghosts, false);
  time_movement = ValidateOutTrimming(time_movement, ghosts, false);

  return time_movement;
}

void TimelineWidget::RippleTool::InitiateGhosts(TimelineViewBlockItem *clicked_item,
                                                Timeline::MovementMode trim_mode,
                                                bool allow_gap_trimming)
{
  Q_UNUSED(allow_gap_trimming)

  PointerTool::InitiateGhosts(clicked_item, trim_mode, true);

  if (parent()->ghost_items_.isEmpty()) {
    return;
  }

  // Find the earliest ripple
  rational earliest_ripple = RATIONAL_MAX;

  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    rational ghost_ripple_point;

    if (trim_mode == Timeline::kTrimIn) {
      ghost_ripple_point = ghost->In();
    } else {
      ghost_ripple_point = ghost->Out();
    }

    earliest_ripple = qMin(earliest_ripple, ghost_ripple_point);
  }

  // For each track that does NOT have a ghost, we need to make one for Gaps
  foreach (TrackOutput* track, parent()->timeline_node_->Tracks()) {
    // Determine if we've already created a ghost on this track
    bool ghost_on_this_track_exists = false;

    foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
      if (parent()->GetTrackFromReference(ghost->Track()) == track) {
        ghost_on_this_track_exists = true;
        break;
      }
    }

    // If there's no ghost on this track, create one
    if (!ghost_on_this_track_exists) {
      // Find the block that starts just before the ripple point, and ends either on or just after it
      Block* block_before_ripple = track->NearestBlockBefore(earliest_ripple);

      // If block is null, there will be no blocks after to ripple
      if (block_before_ripple != nullptr) {
        TimelineViewGhostItem* ghost;

        TrackReference track_ref(track->track_type(), track->Index());

        if (block_before_ripple->type() == Block::kGap) {
          // If this Block is already a Gap, ghost it now
          ghost = AddGhostFromBlock(block_before_ripple, track_ref, trim_mode);
        } else {
          // If there's no gap here, we'll need to create one
          ghost = AddGhostFromNull(block_before_ripple->out(), block_before_ripple->out(), track_ref, trim_mode);
          ghost->setData(TimelineViewGhostItem::kReferenceBlock, Node::PtrToValue(block_before_ripple));
        }

        ghost->SetInvisible(true);
      }
    }
  }
}
