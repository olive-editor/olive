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

#include "node/block/transition/crossdissolve/crossdissolvetransition.h"
#include "node/block/transition/transition.h"
#include "node/factory.h"
#include "transition.h"
#include "widget/nodeview/nodeviewundo.h"
#include "widget/timelinewidget/timelineundo.h"

namespace olive {

TransitionTool::TransitionTool(TimelineWidget *parent) :
  AddTool(parent)
{
}

void TransitionTool::MousePress(TimelineViewMouseEvent *event)
{
  const Track::Reference& track = event->GetTrack();
  Track* t = parent()->GetTrackFromReference(track);
  rational cursor_frame = event->GetFrame();

  if (!t || t->IsLocked()) {
    return;
  }

  Block* block_at_time = t->BlockAtTime(event->GetFrame());
  if (!dynamic_cast<ClipBlock*>(block_at_time)) {
    return;
  }

  // Determine which side of the clip the transition belongs to
  rational transition_start_point;
  Timeline::MovementMode trim_mode;
  rational halfway_point = block_at_time->in() + block_at_time->length() / 2;
  rational tenth_point = block_at_time->in() + block_at_time->length() / 10;
  Block* other_block = nullptr;
  if (cursor_frame < halfway_point) {
    transition_start_point = block_at_time->in();
    trim_mode = Timeline::kTrimIn;

    if (cursor_frame < tenth_point
        && dynamic_cast<ClipBlock*>(block_at_time->previous())) {
      other_block = block_at_time->previous();
    }
  } else {
    transition_start_point = block_at_time->out();
    trim_mode = Timeline::kTrimOut;
    dual_transition_ = (cursor_frame > block_at_time->length() - tenth_point);

    if (cursor_frame > block_at_time->length() - tenth_point
        && dynamic_cast<ClipBlock*>(block_at_time->next())) {
      other_block = block_at_time->next();
    }
  }

  // Create ghost
  ghost_ = new TimelineViewGhostItem();
  ghost_->SetTrack(track);
  ghost_->SetIn(transition_start_point);
  ghost_->SetOut(transition_start_point);
  ghost_->SetMode(trim_mode);
  ghost_->SetData(TimelineViewGhostItem::kAttachedBlock, Node::PtrToValue(block_at_time));

  dual_transition_ = (other_block);
  if (other_block)
    ghost_->SetData(TimelineViewGhostItem::kReferenceBlock, Node::PtrToValue(other_block));

  parent()->AddGhost(ghost_);

  snap_points_.append(transition_start_point);

  // Set the drag start point
  drag_start_point_ = cursor_frame;
}

void TransitionTool::MouseMove(TimelineViewMouseEvent *event)
{
  if (!ghost_) {
    return;
  }

  MouseMoveInternal(event->GetFrame(), dual_transition_);
}

void TransitionTool::MouseRelease(TimelineViewMouseEvent *event)
{
  const Track::Reference& track = ghost_->GetTrack();

  if (ghost_) {
    if (!ghost_->GetAdjustedLength().isNull()) {
      TransitionBlock* transition;

      if (Core::instance()->GetSelectedTransition().isEmpty()) {
        // Fallback if the user hasn't selected one yet
        transition = new CrossDissolveTransition();
      } else {
        transition = static_cast<TransitionBlock*>(NodeFactory::CreateFromID(Core::instance()->GetSelectedTransition()));
      }

      MultiUndoCommand* command = new MultiUndoCommand();

      // Place transition in place
      command->add_child(new NodeAddCommand(static_cast<NodeGraph*>(parent()->GetConnectedNode()->parent()),
                                            transition));

      command->add_child(new TrackPlaceBlockCommand(sequence()->track_list(track.type()),
                                                    track.index(),
                                                    transition,
                                                    ghost_->GetAdjustedIn()));

      if (dual_transition_) {
        transition->set_length_and_media_out(ghost_->GetAdjustedLength());
        transition->set_media_in(-ghost_->GetAdjustedLength()/2);

        // Block mouse is hovering over
        Block* active_block = Node::ValueToPtr<Block>(ghost_->GetData(TimelineViewGhostItem::kAttachedBlock));

        // Block mouse is next to
        Block* friend_block = Node::ValueToPtr<Block>(ghost_->GetData(TimelineViewGhostItem::kReferenceBlock));

        // Use ghost mode to determine which block is which
        Block* out_block = (ghost_->GetMode() == Timeline::kTrimIn) ? friend_block : active_block;
        Block* in_block = (ghost_->GetMode() == Timeline::kTrimIn) ? active_block : friend_block;

        // Connect block to transition
        command->add_child(new NodeEdgeAddCommand(out_block,
                                                  NodeInput(transition, TransitionBlock::kOutBlockInput)));

        command->add_child(new NodeEdgeAddCommand(in_block,
                                                  NodeInput(transition, TransitionBlock::kInBlockInput)));
      } else {
        Block* block_to_transition = Node::ValueToPtr<Block>(ghost_->GetData(TimelineViewGhostItem::kAttachedBlock));
        QString transition_input_to_connect;

        if (ghost_->GetMode() == Timeline::kTrimIn) {
          transition->set_length_and_media_out(ghost_->GetAdjustedLength());
          transition_input_to_connect = TransitionBlock::kInBlockInput;
        } else {
          transition->set_length_and_media_out(ghost_->GetAdjustedLength());
          transition_input_to_connect = TransitionBlock::kOutBlockInput;
        }

        // Connect block to transition
        command->add_child(new NodeEdgeAddCommand(block_to_transition,
                                                  NodeInput(transition, transition_input_to_connect)));
      }

      Core::instance()->undo_stack()->push(command);
    }

    parent()->ClearGhosts();
    snap_points_.clear();
    ghost_ = nullptr;
  }
}

}
