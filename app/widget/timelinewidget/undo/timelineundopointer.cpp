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

#include "timelineundopointer.h"

#include "node/block/gap/gap.h"
#include "node/block/transition/transition.h"
#include "node/graph.h"
#include "timelineundocommon.h"

namespace olive {

//
// BlockTrimCommand
//
void BlockTrimCommand::redo()
{
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
          deleted_adjacent_command_->redo_now();
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
          deleted_adjacent_command_->undo_now();
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

void BlockTrimCommand::prepare()
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

//
// TrackSlideCommand
//
void TrackSlideCommand::redo()
{
  // Make sure all movement blocks' old positions are invalidated
  TimeRange invalidate_range(blocks_.first()->in(), blocks_.last()->out());

  track_->BeginOperation();

  // We will always have an in adjacent if there was a valid slide
  if (we_created_in_adjacent_) {
    // We created in adjacent, so all we have to do is insert it
    in_adjacent_->setParent(track_->parent());
    track_->InsertBlockBefore(in_adjacent_, blocks_.first());
  } else if (-movement_ == in_adjacent_->length()) {
    // Movement will remove in adjacent
    track_->RippleRemoveBlock(in_adjacent_);

    if (NodeCanBeRemoved(in_adjacent_)) {
      if (!in_adjacent_remove_command_) {
        in_adjacent_remove_command_ = CreateRemoveCommand(in_adjacent_);
      }

      in_adjacent_remove_command_->redo_now();
    }
  } else {
    // Simply resize adjacent
    in_adjacent_->set_length_and_media_out(in_adjacent_->length() + movement_);
  }

  // We may not have an out adjacent if the slide was at the end of the track
  if (out_adjacent_) {
    if (we_created_out_adjacent_) {
      // We created out adjacent, so we just have to insert it
      out_adjacent_->setParent(track_->parent());
      track_->InsertBlockAfter(out_adjacent_, blocks_.last());
    } else if (movement_ == out_adjacent_->length()) {
      // Movement will remove out adjacent
      track_->RippleRemoveBlock(out_adjacent_);

      if (NodeCanBeRemoved(out_adjacent_)) {
        if (!out_adjacent_remove_command_) {
          out_adjacent_remove_command_ = CreateRemoveCommand(out_adjacent_);
        }

        out_adjacent_remove_command_->redo_now();
      }
    } else {
      // Simply resize adjacent
      out_adjacent_->set_length_and_media_in(out_adjacent_->length() - movement_);
    }
  }

  track_->EndOperation();

  // Make sure all movement blocks' new positions are invalidated
  invalidate_range.set_range(qMin(invalidate_range.in(), blocks_.first()->in()),
                             qMax(invalidate_range.out(), blocks_.last()->out()));

  track_->Node::InvalidateCache(invalidate_range, Track::kBlockInput);
}


void TrackSlideCommand::undo()
{
  // Make sure all movement blocks' old positions are invalidated
  TimeRange invalidate_range(blocks_.first()->in(), blocks_.last()->out());

  track_->BeginOperation();

  if (we_created_in_adjacent_) {
    // We created this, so we can remove it now
    track_->RippleRemoveBlock(in_adjacent_);
    in_adjacent_->setParent(&memory_manager_);
  } else if (in_adjacent_remove_command_) {
    // We removed this, so we can restore it now
    in_adjacent_remove_command_->undo_now();
  } else {
    // Simply resize adjacent
    in_adjacent_->set_length_and_media_out(in_adjacent_->length() - movement_);
  }

  if (out_adjacent_) {
    if (we_created_out_adjacent_) {
      // We created this, so we can remove it now
      track_->RippleRemoveBlock(out_adjacent_);
      out_adjacent_->setParent(&memory_manager_);
    } else if (out_adjacent_remove_command_) {
      out_adjacent_remove_command_->undo_now();
    } else {
      out_adjacent_->set_length_and_media_in(out_adjacent_->length() + movement_);
    }
  }

  track_->EndOperation();

  // Make sure all movement blocks' new positions are invalidated
  invalidate_range.set_range(qMin(invalidate_range.in(), blocks_.first()->in()),
                             qMax(invalidate_range.out(), blocks_.last()->out()));

  track_->Node::InvalidateCache(invalidate_range, Track::kBlockInput);
}

void TrackSlideCommand::prepare()
{
  if (!in_adjacent_) {
    in_adjacent_ = new GapBlock();
    in_adjacent_->set_length_and_media_out(movement_);
    in_adjacent_->setParent(&memory_manager_);
    we_created_in_adjacent_ = true;
  } else {
    we_created_in_adjacent_ = false;
  }

  if (!out_adjacent_ && blocks_.last()->next()) {
    out_adjacent_ = new GapBlock();
    out_adjacent_->set_length_and_media_out(-movement_);
    out_adjacent_->setParent(&memory_manager_);
    we_created_out_adjacent_ = true;
  } else {
    we_created_out_adjacent_ = false;
  }
}

//
// TrackPlaceBlockCommand
//
TrackPlaceBlockCommand::~TrackPlaceBlockCommand()
{
  delete ripple_remove_command_;
  qDeleteAll(add_track_commands_);
}

void TrackPlaceBlockCommand::redo()
{
  TimeRangeList ranges_to_invalidate;

  // Determine if we need to add tracks
  if (track_index_ >= timeline_->GetTracks().size()) {
    if (add_track_commands_.isEmpty()) {
      // First redo, create tracks now
      add_track_commands_.resize(track_index_ - timeline_->GetTracks().size() + 1);

      for (int i=0; i<add_track_commands_.size(); i++) {
        add_track_commands_[i] = new TimelineAddTrackCommand(timeline_);
      }
    }

    for (int i=0; i<add_track_commands_.size(); i++) {
      add_track_commands_.at(i)->redo_now();
    }
  }

  Track* track = timeline_->GetTrackAt(track_index_);

  track->BeginOperation();

  bool append = (in_ >= track->track_length());

  // Check if the placement location is past the end of the timeline
  if (append) {
    if (in_ > track->track_length()) {
      // If so, insert a gap here
      if (!gap_) {
        gap_ = new GapBlock();
        gap_->set_length_and_media_out(in_ - track->track_length());
      }
      gap_->setParent(track->parent());
      track->AppendBlock(gap_);
      ranges_to_invalidate.insert(gap_->range());
    }

    track->AppendBlock(insert_);
  } else {
    // Place the Block at this point
    if (!ripple_remove_command_) {
      ripple_remove_command_ = new TrackRippleRemoveAreaCommand(track, TimeRange(in_, in_ + insert_->length()));

    }

    ripple_remove_command_->redo_now();
    track->InsertBlockAfter(insert_, ripple_remove_command_->GetInsertionIndex());
  }

  track->EndOperation();

  ranges_to_invalidate.insert(insert_->range());

  foreach (const TimeRange &r, ranges_to_invalidate) {
    track->Node::InvalidateCache(r, Track::kBlockInput);
  }
}

void TrackPlaceBlockCommand::undo()
{
  Track* t = timeline_->GetTrackAt(track_index_);

  TimeRange insert_range(insert_->in(), insert_->out());

  // Firstly, remove our insert
  t->BeginOperation();
  t->RippleRemoveBlock(insert_);

  if (ripple_remove_command_) {
    // If we ripple removed, just undo that
    ripple_remove_command_->undo_now();
  } else if (gap_) {
    t->RippleRemoveBlock(gap_);
    gap_->setParent(&memory_manager_);
  }
  t->EndOperation();

  t->Node::InvalidateCache(insert_range, Track::kBlockInput);

  // Remove tracks if we added them
  for (int i=add_track_commands_.size()-1; i>=0; i--) {
    add_track_commands_.at(i)->undo_now();
  }
}

}
