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

#include "node/block/gap/gap.h"

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

  // Find earliest point to ripple around
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* b = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    if (b == nullptr) {
      // This is a gap we are creating

      // Make sure there's actually a gap being created
      if (ghost->AdjustedLength() > 0) {
        GapBlock* gap = new GapBlock();
        gap->set_length(ghost->AdjustedLength());

        Block* block_to_append_gap_to = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kReferenceBlock));

        parent()->timeline_node_->Tracks().at(ghost->Track())->InsertBlockAfter(gap,
                                                                                block_to_append_gap_to);
      }
    } else {
      // This was a Block that already existed
      if (ghost->AdjustedLength() > 0) {
        b->set_length(ghost->AdjustedLength());

        if (movement_mode == olive::timeline::kTrimIn) {
          // We'll need to shift the media in point too
          b->set_media_in(b->media_in() + ghost->InAdjustment());
        }
      } else {
        // Assumed the Block was a Gap and it was reduced to zero length, remove it here
        parent()->timeline_node_->Tracks().at(ghost->Track())->RippleRemoveBlock(b);
      }
    }
  }
}

rational TimelineView::RippleTool::FrameValidateInternal(rational time_movement, const QVector<TimelineViewGhostItem *> &ghosts)
{
  // Only validate trimming, and we don't care about "overwriting" since the ripple tool is nondestructive
  time_movement = ValidateInTrimming(time_movement, ghosts, false);
  time_movement = ValidateOutTrimming(time_movement, ghosts, false);

  return time_movement;
}

void TimelineView::RippleTool::InitiateGhosts(TimelineViewBlockItem *clicked_item,
                                              olive::timeline::MovementMode trim_mode,
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

    if (trim_mode == olive::timeline::kTrimIn) {
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
      if (ghost->Track() == track->Index()) {
        ghost_on_this_track_exists = true;
        break;
      }
    }

    // If there's no ghost on this track, create one
    if (!ghost_on_this_track_exists) {
      // Find the block that starts just before the ripple point, and ends either on or just after it
      Block* block_before_ripple = track->NearestBlockBefore(earliest_ripple);

      // If block is null, there will be no blocks after to ripple
      if (block_before_ripple != nullptr && block_before_ripple->type() != Block::kEnd) {
        TimelineViewGhostItem* ghost;

        if (block_before_ripple->type() == Block::kGap) {
          // If this Block is already a Gap, ghost it now
          ghost = AddGhostFromBlock(block_before_ripple, track->Index(), trim_mode);
        } else {
          // If there's no gap here, we'll need to create one
          ghost = AddGhostFromNull(block_before_ripple->out(), block_before_ripple->out(), track->Index(), trim_mode);
          ghost->setData(TimelineViewGhostItem::kReferenceBlock, Node::PtrToValue(block_before_ripple));
        }

//        ghost->SetInvisible(true);
      }
    }
  }
}
