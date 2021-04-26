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

#include "timelineundo.h"

namespace olive {

BlockTrimCommand::BlockTrimCommand(Track *track, Block* block, rational new_length, Timeline::MovementMode mode) :
  prepped_(false),
  track_(track),
  block_(block),
  new_length_(new_length),
  mode_(mode),
  deleted_adjacent_command_(nullptr),
  trim_is_a_roll_edit_(false)
{
}

void BlockTrimCommand::redo()
{
  if (!prepped_) {
    prep();
    prepped_ = true;
  }

  if (doing_nothing_) {
    return;
  }

  // Begin an operation since we'll be doing a lot
  track_->BeginOperation();

  // Determine how much time to invalidate
  TimeRange invalidate_range;

  if (mode_ == Timeline::kTrimIn) {
    invalidate_range = TimeRange(block_->in(), block_->in() + trim_diff_);
    block_->set_length_and_media_in(new_length_);
  } else {
    invalidate_range = TimeRange(block_->out(), block_->out() - trim_diff_);
    block_->set_length_and_media_out(new_length_);
  }

  if (needs_adjacent_) {
    if (we_created_adjacent_) {
      // Add adjacent and insert it
      adjacent_->setParent(track_->parent());

      if (mode_ == Timeline::kTrimIn) {
        track_->InsertBlockBefore(adjacent_, block_);
      } else {
        track_->InsertBlockAfter(adjacent_, block_);
      }
    } else if (we_removed_adjacent_) {
      track_->RippleRemoveBlock(adjacent_);

      // It no longer inputs/outputs anything, remove it
      if (remove_block_from_graph_ && NodeCanBeRemoved(adjacent_)) {
        if (!deleted_adjacent_command_) {
          deleted_adjacent_command_ = CreateAndRunRemoveCommand(adjacent_);
        } else {
          deleted_adjacent_command_->redo();
        }
      }
    } else {
      rational adjacent_length = adjacent_->length() + trim_diff_;

      if (mode_ == Timeline::kTrimIn) {
        adjacent_->set_length_and_media_out(adjacent_length);
      } else {
        adjacent_->set_length_and_media_in(adjacent_length);
      }
    }
  }

  track_->EndOperation();

  if (dynamic_cast<TransitionBlock*>(block_)) {
    // Whole transition needs to be invalidated
    invalidate_range = block_->range();
  }

  track_->Node::InvalidateCache(invalidate_range, Track::kBlockInput);
}

void BlockTrimCommand::undo()
{
  if (doing_nothing_) {
    return;
  }

  track_->BeginOperation();

  // Will be POSITIVE if trimming shorter and NEGATIVE if trimming longer
  if (needs_adjacent_) {
    if (we_created_adjacent_) {
      // Adjacent is ours, just delete it
      track_->RippleRemoveBlock(adjacent_);
      adjacent_->setParent(&memory_manager_);
    } else {
      if (we_removed_adjacent_) {
        if (deleted_adjacent_command_) {
          // We deleted adjacent, restore it now
          deleted_adjacent_command_->undo();
        }

        if (mode_ == Timeline::kTrimIn) {
          track_->InsertBlockBefore(adjacent_, block_);
        } else {
          track_->InsertBlockAfter(adjacent_, block_);
        }
      } else {
        rational adjacent_length = adjacent_->length() - trim_diff_;

        if (mode_ == Timeline::kTrimIn) {
          adjacent_->set_length_and_media_out(adjacent_length);
        } else {
          adjacent_->set_length_and_media_in(adjacent_length);
        }
      }
    }
  }

  TimeRange invalidate_range;

  if (mode_ == Timeline::kTrimIn) {
    block_->set_length_and_media_in(old_length_);

    invalidate_range = TimeRange(block_->in(), block_->in() + trim_diff_);
  } else {
    block_->set_length_and_media_out(old_length_);

    invalidate_range = TimeRange(block_->out(), block_->out() - trim_diff_);
  }

  if (dynamic_cast<TransitionBlock*>(block_)) {
    // Whole transition needs to be invalidated
    invalidate_range = block_->range();
  }

  track_->EndOperation();

  track_->Node::InvalidateCache(invalidate_range, Track::kBlockInput);
}

void BlockTrimCommand::prep()
{
  // Store old length
  old_length_ = block_->length();

  // Determine if the length isn't changing, in which case we set a flag to do nothing
  if ((doing_nothing_ = (old_length_ == new_length_))) {
    return;
  }

  // Will be POSITIVE if trimming shorter and NEGATIVE if trimming longer
  trim_diff_ = old_length_ - new_length_;

  // Retrieve our adjacent block (or nullptr if none)
  if (mode_ == Timeline::kTrimIn) {
    adjacent_ = block_->previous();
  } else {
    adjacent_ = block_->next();
  }

  // Ignore when trimming the out with no adjacent, because the user must have trimmed the end
  // of the last block in the track, so we don't need to do anything elses
  needs_adjacent_ = (mode_ == Timeline::kTrimIn || adjacent_);

  if (needs_adjacent_) {
    // If we're trimming shorter, we need an adjacent, so check if we have a viable one.
    we_created_adjacent_ = (trim_diff_ > 0 && (!adjacent_ || (!dynamic_cast<GapBlock*>(adjacent_) && !trim_is_a_roll_edit_)));

    if (we_created_adjacent_) {
      // We shortened but don't have a viable adjacent to lengthen, so we create one
      adjacent_ = new GapBlock();
      adjacent_->set_length_and_media_out(trim_diff_);
    } else {
      // Determine if we're removing the adjacent
      rational adjacent_length = adjacent_->length() + trim_diff_;
      we_removed_adjacent_ = adjacent_length.isNull();
    }
  }
}

void TrackReplaceBlockWithGapCommand::redo()
{
  // Determine if this block is connected to any transitions that should also be removed by this operation
  if (transition_remove_commands_.isEmpty()) {
    CreateRemoveTransitionCommandIfNecessary(false);
    CreateRemoveTransitionCommandIfNecessary(true);
  }
  for (auto it=transition_remove_commands_.cbegin(); it!=transition_remove_commands_.cend(); it++) {
    (*it)->redo();
  }

  if (block_->next()) {
    track_->BeginOperation();

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

      if (!position_command_) {
        position_command_ = new NodeSetPositionAsChildCommand(our_gap_, track_, our_gap_->index(), track_->Blocks().size(), true);
      }
      position_command_->redo();
    }

    track_->EndOperation();

    track_->Node::InvalidateCache(invalidate_range, Track::kBlockInput);

  } else {
    // Block is at the end of the track, simply remove it

    // Determine if it's proceeded by a gap, and remove that gap if so
    Block* preceding = block_->previous();
    if (dynamic_cast<GapBlock*>(preceding)) {
      track_->RippleRemoveBlock(preceding);
      preceding->setParent(&memory_manager_);

      existing_merged_gap_ = static_cast<GapBlock*>(preceding);
    }

    // Remove block in question
    track_->RippleRemoveBlock(block_);
  }
}

void TrackReplaceBlockWithGapCommand::undo()
{
  if (our_gap_ || existing_gap_) {
    track_->BeginOperation();

    if (our_gap_) {

      // We made this gap, simply swap our gap back
      track_->ReplaceBlock(our_gap_, block_);
      our_gap_->setParent(&memory_manager_);

      position_command_->undo();

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

    track_->EndOperation();

    track_->Node::InvalidateCache(TimeRange(block_->in(), block_->out()), Track::kBlockInput);
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
    (*it)->undo();
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

}
