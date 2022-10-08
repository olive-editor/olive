/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "timelineundogeneral.h"

#include "node/block/clip/clip.h"
#include "node/block/transition/transition.h"
#include "node/factory.h"
#include "node/math/math/math.h"
#include "node/math/merge/merge.h"
#include "timelineundocommon.h"
#include "widget/timelinewidget/undo/timelineundotrack.h"

namespace olive {

//
// BlockResizeCommand
//
void BlockResizeCommand::redo()
{
  old_length_ = block_->length();
  block_->set_length_and_media_out(new_length_);
}

void BlockResizeCommand::undo()
{
  block_->set_length_and_media_out(old_length_);
}

//
// BlockResizeWithMediaInCommand
//
void BlockResizeWithMediaInCommand::redo()
{
  old_length_ = block_->length();
  block_->set_length_and_media_in(new_length_);
}

void BlockResizeWithMediaInCommand::undo()
{
  block_->set_length_and_media_in(old_length_);
}

//
// BlockSetMediaInCommand
//
void BlockSetMediaInCommand::redo()
{
  old_media_in_ = block_->media_in();
  block_->set_media_in(new_media_in_);
}

void BlockSetMediaInCommand::undo()
{
  block_->set_media_in(old_media_in_);
}

//
// TimelineAddTrackCommand
//
TimelineAddTrackCommand::TimelineAddTrackCommand(TrackList *timeline, bool automerge_tracks) :
  timeline_(timeline),
  merge_(nullptr),
  position_command_(nullptr)
{
  // Create new track
  track_ = new Track();
  track_->setParent(&memory_manager_);

  // Determine what input to connect it to
  QString relevant_input;

  if (timeline_->type() == Track::kVideo) {
    relevant_input = Sequence::kTextureInput;
  } else if (timeline_->type() == Track::kAudio) {
    relevant_input = Sequence::kSamplesInput;
  }

  // If we have an input to connect to, set it as our `direct` connection
  if (!relevant_input.isEmpty()) {
    direct_ = NodeInput(timeline_->parent(), relevant_input);

    // If we're automerging and something is already connected, determine if/how to merge it
    if (automerge_tracks && direct_.IsConnected()) {
      if (timeline_->type() == Track::kVideo) {
        // Use merge for video
        merge_ = new MergeNode();
        base_ = NodeInput(merge_, MergeNode::kBaseIn);
        blend_ = NodeInput(merge_, MergeNode::kBlendIn);
      } else if (timeline_->type() == Track::kAudio) {
        // Use math (add) for audio
        merge_ = new MathNode();
        base_ = NodeInput(merge_, MathNode::kParamAIn);
        blend_ = NodeInput(merge_, MathNode::kParamBIn);
      }

      if (merge_) {
        // If we got created a merge node, ensure it's parented
        merge_->setParent(&memory_manager_);
      }
    }
  }
}

void TimelineAddTrackCommand::redo()
{
  // Get sequence
  Sequence* sequence = timeline_->parent();

  // Add track to sequence
  track_->setParent(timeline_->GetParentGraph());
  timeline_->ArrayAppend();
  Node::ConnectEdge(track_, timeline_->track_input(timeline_->ArraySize() - 1));

  qreal position_factor = 0.5;
  if (timeline_->type() == Track::kVideo) {
    position_factor = -position_factor;
  }
  bool create_pos_command = (!position_command_ && (timeline_->type() == Track::kVideo || timeline_->type() == Track::kAudio));
  if (create_pos_command) {
    position_command_ = new MultiUndoCommand();
  }

  // Add merge if applicable
  if (merge_) {
    // Determine what was previously connected
    Node *previous_connection = direct_.GetConnectedOutput();

    // Add merge to graph
    merge_->setParent(timeline_->GetParentGraph());

    // Connect merge between what used to be here
    Node::DisconnectEdge(previous_connection, direct_);
    Node::ConnectEdge(merge_, direct_);
    Node::ConnectEdge(previous_connection, base_);
    Node::ConnectEdge(track_, blend_);

    if (create_pos_command) {
      position_command_->add_child(new NodeSetPositionCommand(track_, sequence, sequence->GetNodePositionInContext(sequence) + QPointF(-1, -position_factor)));
      position_command_->add_child(new NodeSetPositionCommand(merge_, sequence, sequence->GetNodePositionInContext(sequence)));
      position_command_->add_child(new NodeSetPositionAndDependenciesRecursivelyCommand(merge_, sequence, sequence->GetNodePositionInContext(sequence) + QPointF(-1, position_factor * timeline_->GetTrackCount())));
    }
  } else if (direct_.IsValid() && !direct_.IsConnected()) {
    // If no merge, we have a direct connection, and nothing else is connected, connect this
    Node::ConnectEdge(track_, direct_);

    if (create_pos_command) {
      // Just position directly next to the context node
      position_command_->add_child(new NodeSetPositionCommand(track_, sequence, sequence->GetNodePositionInContext(sequence) + QPointF(-1, position_factor)));
    }
  }

  // Run position command if we created one
  if (position_command_) {
    position_command_->redo_now();
  }
}

void TimelineAddTrackCommand::undo()
{
  if (position_command_) {
    position_command_->undo_now();
  }

  // Remove merge if applicable
  if (merge_) {
    Node *previous_connection = base_.GetConnectedOutput();

    Node::DisconnectEdge(track_, blend_);
    Node::DisconnectEdge(previous_connection, base_);
    Node::DisconnectEdge(merge_, direct_);
    Node::ConnectEdge(previous_connection, direct_);

    merge_->setParent(&memory_manager_);
  } else if (direct_.IsValid() && direct_.GetConnectedOutput() == track_) {
    Node::DisconnectEdge(track_, direct_);
  }

  // Remove track
  Node::DisconnectEdge(track_, timeline_->track_input(timeline_->ArraySize() - 1));
  timeline_->ArrayRemoveLast();
  track_->setParent(&memory_manager_);
}

//
// TransitionRemoveCommand
//
void TransitionRemoveCommand::redo()
{
  track_ = block_->track();
  out_block_ = block_->connected_out_block();
  in_block_ = block_->connected_in_block();

  Q_ASSERT(out_block_ || in_block_);

  TimeRange invalidate_range(block_->in(), block_->out());

  if (in_block_) {
    in_block_->set_length_and_media_in(in_block_->length() + block_->in_offset());
  }

  if (out_block_) {
    out_block_->set_length_and_media_out(out_block_->length() + block_->out_offset());
  }

  if (in_block_) {
    Node::DisconnectEdge(in_block_, NodeInput(block_, TransitionBlock::kInBlockInput));
  }

  if (out_block_) {
    Node::DisconnectEdge(out_block_, NodeInput(block_, TransitionBlock::kOutBlockInput));
  }

  track_->RippleRemoveBlock(block_);

  if (remove_from_graph_) {
    if (!remove_command_) {
      remove_command_ = CreateRemoveCommand(block_);
    }

    remove_command_->redo_now();
  }
}

void TransitionRemoveCommand::undo()
{
  if (remove_from_graph_) {
    remove_command_->undo_now();
  }

  if (in_block_) {
    track_->InsertBlockBefore(block_, in_block_);
  } else {
    track_->InsertBlockAfter(block_, out_block_);
  }

  if (in_block_) {
    Node::ConnectEdge(in_block_, NodeInput(block_, TransitionBlock::kInBlockInput));
  }

  if (out_block_) {
    Node::ConnectEdge(out_block_, NodeInput(block_, TransitionBlock::kOutBlockInput));
  }

  // These if statements must be separated because in_offset and out_offset report different things
  // if only one block is connected vs two. So we have to connect the blocks first before we have
  // an accurate return value from these offset functions.
  if (in_block_) {
    in_block_->set_length_and_media_in(in_block_->length() - block_->in_offset());
  }

  if (out_block_) {
    out_block_->set_length_and_media_out(out_block_->length() - block_->out_offset());
  }
}

//
// TrackListInsertGaps
//
void TrackListInsertGaps::prepare()
{
  // Determine if all tracks will be affected, which will allow us to make some optimizations
  foreach (Track* track, track_list_->GetTracks()) {
    if (track->IsLocked()) {
      continue;
    }

    working_tracks_.append(track);
  }

  QVector<Block*> blocks_to_split;
  QVector<Block*> blocks_to_append_gap_to;
  QVector<Track*> tracks_to_append_gap_to;

  for (Track* track : qAsConst(working_tracks_)) {
    for (Block* b : track->Blocks()) {
      if (dynamic_cast<GapBlock*>(b) && b->in() <= point_ && b->out() >= point_) {
        // Found a gap at the location
        gaps_to_extend_.append(b);
        break;
      } else if (dynamic_cast<ClipBlock*>(b) && b->out() >= point_) {
        bool append_gap = true;

        if (b->in() == point_) {
          // The only reason we should be here is if this block is at the start of the track,
          // in which case no split needs to occur
          b = nullptr;
        } else if (b->out() > point_) {
          // Block must be split as well as having a gap appended to it
          blocks_to_split.append(b);
        } else if (!b->next()) {
          // At the end of a track, no gap needs to be added at all
          append_gap = false;
        }

        if (append_gap) {
          tracks_to_append_gap_to.append(track);
          blocks_to_append_gap_to.append(b);
        }
        break;
      }
    }
  }

  if (!blocks_to_split.isEmpty()) {
    split_command_ = new BlockSplitPreservingLinksCommand(blocks_to_split, {point_});
  }

  for (int i=0; i<blocks_to_append_gap_to.size(); i++) {
    GapBlock* gap = new GapBlock();
    gap->set_length_and_media_out(length_);
    gap->setParent(&memory_manager_);
    gaps_added_.append({gap, blocks_to_append_gap_to.at(i), tracks_to_append_gap_to.at(i)});
  }
}

void TrackListInsertGaps::redo()
{
  foreach (Block* gap, gaps_to_extend_) {
    gap->set_length_and_media_out(gap->length() + length_);
  }

  if (split_command_) {
    split_command_->redo_now();
  }

  foreach (auto add_gap, gaps_added_) {
    add_gap.gap->setParent(add_gap.track->parent());
    add_gap.track->InsertBlockAfter(add_gap.gap, add_gap.before);
  }
}

void TrackListInsertGaps::undo()
{
  // Remove added gaps
  foreach (auto add_gap, gaps_added_) {
    add_gap.gap->track()->RippleRemoveBlock(add_gap.gap);
    add_gap.gap->setParent(&memory_manager_);
  }

  // Un-split blocks
  if (split_command_) {
    split_command_->undo_now();
  }

  // Restore original length of gaps
  foreach (Block* gap, gaps_to_extend_) {
    gap->set_length_and_media_out(gap->length() - length_);
  }
}

//
// TrackReplaceBlockWithGapCommand
//
void TrackReplaceBlockWithGapCommand::redo()
{
  // Determine if this block is connected to any transitions that should also be removed by this operation
  if (handle_transitions_ && transition_remove_commands_.isEmpty()) {
    CreateRemoveTransitionCommandIfNecessary(false);
    CreateRemoveTransitionCommandIfNecessary(true);
  }
  for (auto it=transition_remove_commands_.cbegin(); it!=transition_remove_commands_.cend(); it++) {
    (*it)->redo_now();
  }

  if (block_->next()) {
    // Invalidate the range inhabited by this block
    TimeRange invalidate_range(block_->in(), block_->out());

    // Block has a next, which means it's NOT at the end of the sequence and thus requires a gap
    rational new_gap_length = block_->length();

    Block* previous = block_->previous();
    Block* next = block_->next();

    bool previous_is_a_gap = dynamic_cast<GapBlock*>(previous);
    bool next_is_a_gap = dynamic_cast<GapBlock*>(next);

    if (previous_is_a_gap && next_is_a_gap) {
      // Clip is preceded and followed by a gap, so we'll merge the two
      existing_gap_ = static_cast<GapBlock*>(previous);

      existing_merged_gap_ = static_cast<GapBlock*>(next);
      new_gap_length += existing_merged_gap_->length();
      track_->RippleRemoveBlock(existing_merged_gap_);
      existing_merged_gap_->setParent(&memory_manager_);
    } else if (previous_is_a_gap) {
      // Extend this gap to fill space left by block
      existing_gap_ = static_cast<GapBlock*>(previous);
    } else if (next_is_a_gap) {
      // Extend this gap to fill space left by block
      existing_gap_ = static_cast<GapBlock*>(next);
    }

    if (existing_gap_) {
      // Extend an existing gap
      new_gap_length += existing_gap_->length();
      existing_gap_->set_length_and_media_out(new_gap_length);
      track_->RippleRemoveBlock(block_);

      existing_gap_precedes_ = (existing_gap_ == previous);
    } else {
      // No gap exists to fill this space, create a new one and swap it in
      if (!our_gap_) {
        our_gap_ = new GapBlock();
        our_gap_->set_length_and_media_out(new_gap_length);
      }

      our_gap_->setParent(track_->parent());
      track_->ReplaceBlock(block_, our_gap_);
    }

  } else {
    // Block is at the end of the track, simply remove it
    Block* preceding = block_->previous();
    track_->RippleRemoveBlock(block_);

    // Determine if it's preceded by a gap, and remove that gap if so
    if (dynamic_cast<GapBlock*>(preceding)) {
      track_->RippleRemoveBlock(preceding);
      preceding->setParent(&memory_manager_);

      existing_merged_gap_ = static_cast<GapBlock*>(preceding);
    }
  }
}

void TrackReplaceBlockWithGapCommand::undo()
{
  if (our_gap_ || existing_gap_) {
    if (our_gap_) {

      // We made this gap, simply swap our gap back
      track_->ReplaceBlock(our_gap_, block_);
      our_gap_->setParent(&memory_manager_);

    } else {

      // If we're here, assume that we extended an existing gap
      rational original_gap_length = existing_gap_->length() - block_->length();

      // If we merged two gaps together, restore the second one now
      if (existing_merged_gap_) {
        original_gap_length -= existing_merged_gap_->length();
        existing_merged_gap_->setParent(track_->parent());
        track_->InsertBlockAfter(existing_merged_gap_, existing_gap_);
        existing_merged_gap_ = nullptr;
      }

      // Restore original block
      if (existing_gap_precedes_) {
        track_->InsertBlockAfter(block_, existing_gap_);
      } else {
        track_->InsertBlockBefore(block_, existing_gap_);
      }

      // Restore gap's original length
      existing_gap_->set_length_and_media_out(original_gap_length);

      existing_gap_ = nullptr;

    }

  } else {

    // Our gap and existing gap were both null, our block must have been at the end and thus
    // required no gap extension/replacement

    // However, we may have removed an unnecessary gap that preceded it
    if (existing_merged_gap_) {
      existing_merged_gap_->setParent(track_->parent());
      track_->AppendBlock(existing_merged_gap_);
      existing_merged_gap_ = nullptr;
    }

    // Restore block
    track_->AppendBlock(block_);

  }

  for (auto it=transition_remove_commands_.crbegin(); it!=transition_remove_commands_.crend(); it++) {
    (*it)->undo_now();
  }
}

void TrackReplaceBlockWithGapCommand::CreateRemoveTransitionCommandIfNecessary(bool next)
{
  Block* relevant_block;

  if (next) {
    relevant_block = block_->next();
  } else {
    relevant_block = block_->previous();
  }

  TransitionBlock* transition_cast_test = dynamic_cast<TransitionBlock*>(relevant_block);

  if (transition_cast_test) {
    if ((next && transition_cast_test->connected_out_block() == block_ && !transition_cast_test->connected_in_block())
        || (!next && transition_cast_test->connected_in_block() == block_ && !transition_cast_test->connected_out_block())) {
      TransitionRemoveCommand* command = new TransitionRemoveCommand(transition_cast_test, true);
      transition_remove_commands_.append(command);
    }
  }
}

void TimelineRemoveTrackCommand::prepare()
{
  list_ = track_->sequence()->track_list(track_->type());

  index_ = list_->GetArrayIndexFromCacheIndex(track_->Index());

  remove_command_ = new NodeRemoveWithExclusiveDependenciesAndDisconnect(track_);
}

void TimelineRemoveTrackCommand::redo()
{
  remove_command_->redo_now();

  list_->parent()->InputArrayRemove(list_->track_input(), index_);
}

void TimelineRemoveTrackCommand::undo()
{
  list_->parent()->InputArrayInsert(list_->track_input(), index_);

  remove_command_->undo_now();
}

void TimelineAddDefaultTransitionCommand::prepare()
{
  for (auto it=clips_.cbegin(); it!=clips_.cend(); it++) {
    ClipBlock *c = *it;

    // Handle in transition
    if (clips_.contains(static_cast<ClipBlock*>(c->previous()))) {
      // Do nothing, assume this will be handled by a dual transition from that clip
    } else if (dynamic_cast<GapBlock*>(c->previous()) || !c->previous()) {
      // Create in transition
      AddTransition(c, kIn);
    }

    // Handle out transition
    if (clips_.contains(static_cast<ClipBlock*>(c->next()))) {
      AddTransition(c, kOutDual);
    } else if (dynamic_cast<GapBlock*>(c->next()) || !c->next()) {
      // Create out transition
      AddTransition(c, kOut);
    }
  }
}

void TimelineAddDefaultTransitionCommand::AddTransition(ClipBlock *c, CreateTransitionMode mode)
{
  if (Track *t = c->track()) {
    Node *p = nullptr;
    if (t->type() == Track::kVideo) {
      p = NodeFactory::CreateFromID(OLIVE_CONFIG("DefaultVideoTransition").toString());
    } else if (t->type() == Track::kAudio) {
      p = NodeFactory::CreateFromID(OLIVE_CONFIG("DefaultAudioTransition").toString());
    }

    rational transition_length = OLIVE_CONFIG("DefaultTransitionLength").value<rational>();

    // Resize original clip
    switch (mode) {
    case kIn:
      ValidateTransitionLength(c, transition_length);

      if (transition_length > 0) {
        AdjustClipLength(c, transition_length, false);
      }
      break;
    case kOut:
      ValidateTransitionLength(c, transition_length);

      if (transition_length > 0) {
        AdjustClipLength(c, transition_length, true);
      }
      break;
    case kOutDual:
    {
      rational half_length = transition_length / 2;

      ValidateTransitionLength(static_cast<ClipBlock*>(c->next()), half_length);
      ValidateTransitionLength(c, half_length);

      transition_length = half_length * 2;

      if (transition_length > 0) {
        AdjustClipLength(static_cast<ClipBlock*>(c->next()), half_length, false);
        AdjustClipLength(c, half_length, true);
      }
      break;
    }
    }

    if (transition_length > 0) {
      if (TransitionBlock *transition = dynamic_cast<TransitionBlock*>(p)) {
        transition->set_length_and_media_out(transition_length);

        // Add transition
        commands_.append(new NodeAddCommand(c->parent(), transition));

        // Insert block
        Block *insert_after = (mode == kIn) ? c->previous() : c;
        commands_.append(new TrackInsertBlockAfterCommand(c->track(), transition, insert_after));

        // Connect
        switch (mode) {
        case kIn:
          commands_.append(new NodeEdgeAddCommand(c, NodeInput(transition, TransitionBlock::kInBlockInput)));
          break;
        case kOutDual:
          commands_.append(new NodeEdgeAddCommand(c->next(), NodeInput(transition, TransitionBlock::kInBlockInput)));
          /* fall through */
        case kOut:
          commands_.append(new NodeEdgeAddCommand(c, NodeInput(transition, TransitionBlock::kOutBlockInput)));
          break;
        }
      }
    }
  }
}

void TimelineAddDefaultTransitionCommand::AdjustClipLength(ClipBlock *c, const rational &transition_length, bool out)
{
  rational cur_len = lengths_.value(c, c->length());
  rational new_len = cur_len - transition_length;
  if (out) {
    commands_.append(new BlockResizeCommand(c, new_len));
  } else {
    commands_.append(new BlockResizeWithMediaInCommand(c, new_len));
  }
  lengths_.insert(c, new_len);
}

void TimelineAddDefaultTransitionCommand::ValidateTransitionLength(ClipBlock *c, rational &transition_length)
{
  rational cur_len = lengths_.value(c, c->length());
  rational half_cur_len = cur_len/2;
  if (transition_length >= half_cur_len) {
    transition_length = half_cur_len - timebase_;
  }
}

}
