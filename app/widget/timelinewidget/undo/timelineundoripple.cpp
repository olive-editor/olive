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

#include "timelineundoripple.h"

#include "timelineundocommon.h"

namespace olive {

//
// TrackRippleRemoveAreaCommand
//
TrackRippleRemoveAreaCommand::TrackRippleRemoveAreaCommand(Track* track, const TimeRange& range) :
  track_(track),
  range_(range),
  splice_split_command_(nullptr)
{
  trim_out_.block = nullptr;
  trim_in_.block = nullptr;
}

TrackRippleRemoveAreaCommand::~TrackRippleRemoveAreaCommand()
{
  delete splice_split_command_;
  qDeleteAll(remove_block_commands_);
}

void TrackRippleRemoveAreaCommand::prepare()
{
  // Determine precisely what will be happening to these tracks
  Block* first_block = track_->NearestBlockBeforeOrAt(range_.in());

  if (!first_block) {
    // No blocks at this time, nothing to be done on this track
    return;
  }

  // Determine if this first block is getting trimmed or removed
  bool first_block_is_out_trimmed = first_block->in() < range_.in();
  bool first_block_is_in_trimmed = first_block->out() > range_.out();

  // Set's the block that any insert command should insert AFTER. If the first block is not
  // getting out-trimmed, that means first block is either getting removed or in-trimmed, which
  // means any insert should happen before it
  insert_previous_ = first_block_is_out_trimmed ? first_block : first_block->previous();

  // If it's getting trimmed, determine if it's actually getting spliced
  if (first_block_is_out_trimmed && first_block_is_in_trimmed) {
    // This block is getting spliced, so we'll handle that later
    splice_split_command_ = new BlockSplitCommand(first_block, range_.in());
  } else {
    // It's just getting trimmed or removed, so we'll append that operation
    if (first_block_is_out_trimmed) {
      trim_out_ = {first_block,
                   first_block->length(),
                   first_block->length() - (first_block->out() - range_.in())};
    } else if (first_block_is_in_trimmed) {
      // Block is getting in trimmed
      trim_in_ = {first_block,
                 first_block->length(),
                 first_block->length() - (range_.out() - first_block->in())};
    } else {
      // We know for sure this block is within the range so it will be removed
      removals_.append(RemoveOperation({first_block, first_block->previous()}));
    }

    // If the first block is getting in trimmed, we're already at the end of our range
    if (!first_block_is_in_trimmed) {
      // Loop through the rest of the blocks and determine what to do with those
      for (Block* next=first_block->next(); next; next=next->next()) {
        bool trimming = (next->out() > range_.out());

        if (trimming) {
          trim_in_ = {next,
                      next->length(),
                      next->length() - (range_.out() - next->in())};
          break;
        } else {
          removals_.append(RemoveOperation({next, next->previous()}));

          if (next->out() == range_.out()) {
            break;
          }
        }
      }
    }
  }
}

void TrackRippleRemoveAreaCommand::redo()
{
  track_->BeginOperation();

  if (splice_split_command_) {
    // We're just splicing
    splice_split_command_->redo_now();

    // Trim the in of the split
    Block* split = splice_split_command_->new_block();
    split->set_length_and_media_in(split->length() - (range_.out() - split->in()));
  } else {
    if (trim_out_.block) {
      trim_out_.block->set_length_and_media_out(trim_out_.new_length);
    }

    if (trim_in_.block) {
      trim_in_.block->set_length_and_media_in(trim_in_.new_length);
    }

    // Perform removals
    if (!removals_.isEmpty()) {
      foreach (auto op, removals_) {
        // Ripple remove them all first
        track_->RippleRemoveBlock(op.block);
      }

      // Create undo commands for node removals where possible
      if (remove_block_commands_.isEmpty()) {
        foreach (auto op, removals_) {
          if (NodeCanBeRemoved(op.block)) {
            remove_block_commands_.append(CreateRemoveCommand(op.block));
          }
        }
      }

      foreach (UndoCommand* c, remove_block_commands_) {
        c->redo_now();
      }
    }
  }

  track_->EndOperation();

  track_->Node::InvalidateCache(TimeRange(range_.in(), RATIONAL_MAX), Track::kBlockInput);
}

void TrackRippleRemoveAreaCommand::undo()
{
  // Begin operations
  track_->BeginOperation();

  if (splice_split_command_) {
    splice_split_command_->undo_now();
  } else {
    if (trim_out_.block) {
      trim_out_.block->set_length_and_media_out(trim_out_.old_length);
    }

    if (trim_in_.block) {
      trim_in_.block->set_length_and_media_in(trim_in_.old_length);
    }

    // Un-remove any blocks
    for (int i=remove_block_commands_.size()-1; i>=0; i--) {
      remove_block_commands_.at(i)->undo_now();
    }

    foreach (auto op, removals_) {
      track_->InsertBlockAfter(op.block, op.before);
    }
  }

  // End operations and invalidate
  track_->EndOperation();

  track_->Node::InvalidateCache(TimeRange(range_.in(), RATIONAL_MAX), Track::kBlockInput);
}

//
// TrackListRippleRemoveAreaCommand
//
void TrackListRippleRemoveAreaCommand::redo()
{
  // Code that's only run on the first redo
  if (commands_.isEmpty()) {
    all_tracks_unlocked_ = true;

    foreach (Track* track, list_->GetTracks()) {
      if (track->IsLocked()) {
        all_tracks_unlocked_ = false;
        continue;
      }

      TrackRippleRemoveAreaCommand* c = new TrackRippleRemoveAreaCommand(track, range_);
      commands_.append(c);
      working_tracks_.append(track);
    }
  }

  if (all_tracks_unlocked_) {
    // We can optimize here by simply shifting the whole cache forward instead of re-caching
    // everything following this time
    if (list_->type() == Track::kVideo) {
      list_->parent()->ShiftVideoCache(range_.out(), range_.in());
    } else if (list_->type() == Track::kAudio) {
      list_->parent()->ShiftAudioCache(range_.out(), range_.in());
    }

    foreach (Track* track, working_tracks_) {
      track->BeginOperation();
    }
  }

  foreach (TrackRippleRemoveAreaCommand* c, commands_) {
    c->redo_now();
  }

  if (all_tracks_unlocked_) {
    foreach (Track* track, working_tracks_) {
      track->EndOperation();
    }
  }
}

void TrackListRippleRemoveAreaCommand::undo()
{
  if (all_tracks_unlocked_) {
    // We can optimize here by simply shifting the whole cache forward instead of re-caching
    // everything following this time
    if (list_->type() == Track::kVideo) {
      list_->parent()->ShiftVideoCache(range_.in(), range_.out());
    } else if (list_->type() == Track::kAudio) {
      list_->parent()->ShiftAudioCache(range_.in(), range_.out());
    }

    foreach (Track* track, working_tracks_) {
      track->BeginOperation();
    }
  }

  foreach (TrackRippleRemoveAreaCommand* c, commands_) {
    c->undo_now();
  }

  if (all_tracks_unlocked_) {
    foreach (Track* track, working_tracks_) {
      track->EndOperation();
      track->Node::InvalidateCache(range_, Track::kBlockInput);
    }
  }
}

//
// TimelineRippleRemoveAreaCommand
//
TimelineRippleRemoveAreaCommand::TimelineRippleRemoveAreaCommand(Sequence* timeline, rational in, rational out) :
  timeline_(timeline)
{
  for (int i=0; i<Track::kCount; i++) {
    add_child(new TrackListRippleRemoveAreaCommand(timeline->track_list(static_cast<Track::Type>(i)),
                                                   in,
                                                   out));
  }
}

//
// TrackListRippleToolCommand
//
TrackListRippleToolCommand::TrackListRippleToolCommand(TrackList* track_list,
                           const QHash<Track*, RippleInfo>& info,
                           const rational& ripple_movement,
                           const Timeline::MovementMode& movement_mode) :
  track_list_(track_list),
  info_(info),
  ripple_movement_(ripple_movement),
  movement_mode_(movement_mode)
{
  all_tracks_unlocked_ = (info_.size() == track_list_->GetTrackCount());
}

void TrackListRippleToolCommand::ripple(bool redo)
{
  if (info_.isEmpty()) {
    return;
  }

  // The following variables are used to determine how much of the cache to invalidate

  // If we can shift, we will shift from the latest out before the ripple to the latest out after,
  // since those sections will be unchanged by this ripple
  rational pre_latest_out = RATIONAL_MIN;
  rational post_latest_out = RATIONAL_MIN;

  // Make timeline changes
  for (auto it=info_.cbegin(); it!=info_.cend(); it++) {
    Track* track = it.key();
    const RippleInfo& info = it.value();
    WorkingData working_data = working_data_.value(track);
    Block* b = info.block;

    // Generate block length
    rational new_block_length;
    rational operation_movement = ripple_movement_;

    if (movement_mode_ == Timeline::kTrimIn) {
      operation_movement = -operation_movement;
    }

    if (!redo) {
      operation_movement = -operation_movement;
    }

    if (b) {
      new_block_length = b->length() + operation_movement;
    }

    rational pre_shift;
    rational post_shift;

    // Begin operation so we can invalidate better later
    track->BeginOperation();

    if (info.append_gap) {

      // Rather than rippling the referenced block, we'll insert a gap and ripple with that
      GapBlock* gap = working_data.created_gap;

      if (redo) {
        if (!gap) {
          gap = new GapBlock();
          gap->set_length_and_media_out(qAbs(ripple_movement_));
          working_data.created_gap = gap;
        }

        gap->setParent(track->parent());
        track->InsertBlockBefore(gap, b);

        // As an insertion, we will shift from the gap's in to the gap's out
        pre_shift = gap->in();
        post_shift = gap->out();
        working_data.earliest_point_of_change = gap->in();
      } else {
        // As a removal, we will shift from the gap's out to the gap's in
        pre_shift = gap->out();
        post_shift = gap->in();

        track->RippleRemoveBlock(gap);
        gap->setParent(&memory_manager_);
      }

    } else if ((redo && new_block_length.isNull()) || (!redo && !b->track())) {

      // The ripple is the length of this block. We assume that for this to happen, it must have
      // been a gap that we will now remove.

      if (redo) {
        // The earliest point changes will happen is at the start of this block
        working_data.earliest_point_of_change = b->in();

        // As a removal, we will be shifting from the out point to the in point
        pre_shift = b->out();
        post_shift = b->in();

        // Remove gap from track and from graph
        working_data.removed_gap_after = b->previous();
        track->RippleRemoveBlock(b);
        b->setParent(&memory_manager_);
      } else {
        // Restore gap to graph and track
        b->setParent(track->parent());
        track->InsertBlockAfter(b, working_data.removed_gap_after);

        // The earliest point changes will happen is at the start of this block
        working_data.earliest_point_of_change = b->in();

        // As an insert, we will be shifting from the block's in point to its out point
        pre_shift = b->in();
        post_shift = b->out();
      }

    } else {

      // Store old length
      working_data.old_length = b->length();

      if (movement_mode_ == Timeline::kTrimIn) {
        // The earliest point changes will occur is in point of this bloc
        working_data.earliest_point_of_change = b->in();

        // Undo the trim in inversion we do above, this will still be inverted accurately for
        // undoing where appropriate
        rational inverted = -operation_movement;
        if (inverted > 0) {
          pre_shift = b->in() + inverted;
          post_shift = b->in();
        } else {
          pre_shift = b->in();
          post_shift = b->in() - inverted;
        }

        // Update length
        b->set_length_and_media_in(new_block_length);
      } else {
        // The earliest point changes will occur is the out point if trimming out or the in point
        // if trimming in
        working_data.earliest_point_of_change = b->out();

        // The latest out before the ripple is this block's current out point
        pre_shift = b->out();

        // Update length
        b->set_length_and_media_out(new_block_length);

        // The latest out after the ripple is this block's out point after the length change
        post_shift = b->out();
      }

    }

    working_data_.insert(it.key(), working_data);

    pre_latest_out = qMax(pre_latest_out, pre_shift);
    post_latest_out = qMax(post_latest_out, post_shift);
  }

  if (all_tracks_unlocked_) {
    // We rippled all the tracks, so we can shift the whole cache
    if (track_list_->type() == Track::kVideo) {
      track_list_->parent()->ShiftVideoCache(pre_latest_out, post_latest_out);
    } else if (track_list_->type() == Track::kAudio) {
      track_list_->parent()->ShiftAudioCache(pre_latest_out, post_latest_out);
    }
  }

  for (auto it=working_data_.cbegin(); it!=working_data_.cend(); it++) {
    Track* track = it.key();

    track->EndOperation();

    if (!all_tracks_unlocked_) {
      // If we're not shifting, the whole track must get invalidated
      track->Node::InvalidateCache(TimeRange(it.value().earliest_point_of_change, RATIONAL_MAX), Track::kBlockInput);
    } else if (pre_latest_out < post_latest_out) {
      // If we're here, then a new section has been rippled in that needs to be rendered
      track->Node::InvalidateCache(TimeRange(pre_latest_out, post_latest_out), Track::kBlockInput);
    }
  }
}

//
// TimelineRippleDeleteGapsAtRegionsCommand
//
void TimelineRippleDeleteGapsAtRegionsCommand::prepare()
{
  int max_gaps = 0;
  QHash<Track*, QVector<RemovalRequest> > requested_gaps;

  // Convert regions to gaps
  for (const QPair<Track*, TimeRange> &region : qAsConst(regions_)) {
    Track *track = region.first;
    const TimeRange &range = region.second;

    GapBlock *gap = dynamic_cast<GapBlock*>(track->NearestBlockBeforeOrAt(range.in()));

    if (gap) {
      QVector<RemovalRequest> &gaps_on_track = requested_gaps[track];

      RemovalRequest this_req = {gap, range};

      // Insertion sort
      bool inserted = false;
      for (int i=0; i<gaps_on_track.size(); i++) {
        if (gaps_on_track.at(i).range.in() < range.in()) {
          gaps_on_track.insert(i, this_req);
          inserted = true;
          break;
        }
      }
      if (!inserted) {
        gaps_on_track.append(this_req);
      }

      max_gaps = qMax(max_gaps, gaps_on_track.size());
    } else {
      qWarning() << "Failed to find corresponding gap to region";
    }
  }

  // For each gap on each track, find a corresponding gap on every other track (which may include
  // a requested gap) to ripple in order to keep everything synchronized
  QHash<GapBlock*, rational> gap_lengths;
  for (int gap_index=0; gap_index<max_gaps; gap_index++) {
    rational earliest_point = RATIONAL_MAX;
    rational ripple_length = RATIONAL_MAX;
    rational latest_point = RATIONAL_MIN;

    foreach (const QVector<RemovalRequest> &gaps_on_track, requested_gaps) {
      if (gap_index < gaps_on_track.size()) {
        const RemovalRequest &gap = gaps_on_track.at(gap_index);
        earliest_point = qMin(earliest_point, gap.range.in());
        ripple_length = qMin(ripple_length, gap.range.length());
        latest_point = qMax(latest_point, gap.range.out());
      }
    }

    // Determine which gaps will be involved in this operation
    QVector<GapBlock*> gaps;

    bool all_tracks_unlocked = true;

    foreach (Track* track, timeline_->GetTracks()) {
      if (track->IsLocked()) {
        all_tracks_unlocked = false;
        continue;
      }

      const QVector<RemovalRequest> &requested_gaps_on_track = requested_gaps.value(track);
      GapBlock *gap = nullptr;
      if (gap_index < requested_gaps_on_track.size()) {
        // A requested gap was at this index, use it
        gap = requested_gaps_on_track.at(gap_index).gap;
      } else {
        // No requested gap was at this index, find one
        Block *block = track->NearestBlockAfterOrAt(earliest_point);

        if (block) {
          // Found a block, test if it's a gap
          gap = dynamic_cast<GapBlock*>(block);

          if (!gap) {
            if (block->in() == earliest_point) {
              if (block->next()) {
                gap = dynamic_cast<GapBlock*>(block->next());

                if (!gap) {
                  ripple_length = 0;
                }
              }
            } else {
              gap = dynamic_cast<GapBlock*>(block->previous());

              if (!gap) {
                ripple_length = 0;
              }
            }
          }
        } else {
          // Assume track finishes here and track won't be affected by this operation
        }
      }

      if (gap) {
        gaps.append(gap);

        if (!gap_lengths.contains(gap)) {
          gap_lengths.insert(gap, gap->length());
        }

        ripple_length = qMin(ripple_length, gap_lengths.value(gap));
      }

      if (ripple_length == 0) {
        break;
      }
    }

    if (ripple_length > 0) {
      foreach (GapBlock *gap, gaps) {
        if (all_tracks_unlocked) {
          commands_.append(new NodeBeginOperationCommand(gap->track()));
        }

        if (gap_lengths.value(gap) == ripple_length) {
          commands_.append(new TrackRippleRemoveBlockCommand(gap->track(), gap));
        } else {
          gap_lengths[gap] -= ripple_length;
          commands_.append(new BlockResizeCommand(gap, gap_lengths.value(gap)));
        }

        if (all_tracks_unlocked) {
          commands_.append(new NodeEndOperationCommand(gap->track()));
        }
      }

      if (all_tracks_unlocked) {
        commands_.append(new TimelineShiftCacheCommand(timeline_, latest_point, latest_point - ripple_length));
      }
    }
  }
}

void TimelineRippleDeleteGapsAtRegionsCommand::redo()
{
  for (auto it=commands_.cbegin(); it!=commands_.cend(); it++) {
    (*it)->redo_now();
  }
}

void TimelineRippleDeleteGapsAtRegionsCommand::undo()
{
  for (auto it=commands_.crbegin(); it!=commands_.crend(); it++) {
    (*it)->undo_now();
  }
}

void TimelineShiftCacheCommand::redo()
{
  timeline_->ShiftCache(from_, to_);
}

void TimelineShiftCacheCommand::undo()
{
  timeline_->ShiftCache(to_, from_);
}

}
