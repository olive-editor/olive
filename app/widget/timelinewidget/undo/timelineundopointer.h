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

#ifndef TIMELINEUNDOPOINTER_H
#define TIMELINEUNDOPOINTER_H

#include "node/block/gap/gap.h"
#include "node/output/track/track.h"
#include "node/output/track/tracklist.h"
#include "node/project/sequence/sequence.h"
#include "timelineundogeneral.h"
#include "timelineundoripple.h"

namespace olive {

/**
 * @brief Performs a trim in the timeline that only affects the block and the block adjacent
 *
 * Changes the length of one block while also changing the length of the block directly adjacent
 * to compensate so that the rest of the track is unaffected.
 *
 * By default, this will only affect the length of gaps. If the adjacent needs to increase its
 * length and is not a gap, a gap will be created and inserted to fill that time. This command can
 * be set to always trim even if the adjacent clip isn't a gap with SetTrimIsARollEdit()
 */
class BlockTrimCommand : public UndoCommand {
public:
  BlockTrimCommand(Track *track, Block* block, rational new_length, Timeline::MovementMode mode) :
    track_(track),
    block_(block),
    new_length_(new_length),
    mode_(mode),
    deleted_adjacent_command_(nullptr),
    trim_is_a_roll_edit_(false)
  {
  }

  virtual ~BlockTrimCommand() override
  {
    delete deleted_adjacent_command_;
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_->project();
  }

  /**
   * @brief Set this if the trim should always affect the adjacent clip and not create a gap
   */
  void SetTrimIsARollEdit(bool e)
  {
    trim_is_a_roll_edit_ = e;
  }

  /**
   * @brief Set whether adjacent blocks set to zero length should be removed from the whole graph
   *
   * If an adjacent block's length is set to 0, it's automatically removed from the track. By
   * default it also gets removed from the whole graph. Set this to FALSE to disable that
   * functionality.
   */
  void SetRemoveZeroLengthFromGraph(bool e)
  {
    remove_block_from_graph_ = e;
  }

protected:
  virtual void prepare() override;
  virtual void redo() override;
  virtual void undo() override;

private:
  bool doing_nothing_;
  rational trim_diff_;

  Track* track_;
  Block* block_;
  rational old_length_;
  rational new_length_;
  Timeline::MovementMode mode_;

  Block* adjacent_;
  bool needs_adjacent_;
  bool we_created_adjacent_;
  bool we_removed_adjacent_;
  UndoCommand* deleted_adjacent_command_;

  bool trim_is_a_roll_edit_;
  bool remove_block_from_graph_;

  QObject memory_manager_;

};

class TrackSlideCommand : public UndoCommand {
public:
  TrackSlideCommand(Track* track, const QList<Block*>& moving_blocks, Block* in_adjacent, Block* out_adjacent, const rational& movement) :
    track_(track),
    blocks_(moving_blocks),
    movement_(movement),
    in_adjacent_(in_adjacent),
    in_adjacent_remove_command_(nullptr),
    out_adjacent_(out_adjacent),
    out_adjacent_remove_command_(nullptr)
  {
    Q_ASSERT(!movement_.isNull());
  }

  virtual ~TrackSlideCommand() override
  {
    delete in_adjacent_remove_command_;
    delete out_adjacent_remove_command_;
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_->project();
  }

protected:
  virtual void prepare() override;

  virtual void redo() override;

  virtual void undo() override;

private:
  Track* track_;
  QList<Block*> blocks_;
  rational movement_;

  bool we_created_in_adjacent_;
  Block* in_adjacent_;
  UndoCommand* in_adjacent_remove_command_;
  bool we_created_out_adjacent_;
  Block* out_adjacent_;
  UndoCommand* out_adjacent_remove_command_;

  QObject memory_manager_;

};

/**
 * @brief Destructively places `block` at the in point `start`
 *
 * The Block is guaranteed to be placed at the starting point specified. If there are Blocks in this area, they are
 * either trimmed or removed to make space for this Block. Additionally, if the Block is placed beyond the end of
 * the Sequence, a GapBlock is inserted to compensate.
 */
class TrackPlaceBlockCommand : public UndoCommand {
public:
  TrackPlaceBlockCommand(TrackList *timeline, int track, Block* block, rational in) :
    timeline_(timeline),
    track_index_(track),
    in_(in),
    gap_(nullptr),
    insert_(block),
    ripple_remove_command_(nullptr)
  {
  }

  virtual ~TrackPlaceBlockCommand() override;

  virtual Project* GetRelevantProject() const override
  {
    return timeline_->parent()->project();
  }

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  TrackList* timeline_;
  int track_index_;
  rational in_;
  GapBlock* gap_;
  Block* insert_;
  QVector<TimelineAddTrackCommand*> add_track_commands_;
  QObject memory_manager_;
  TrackRippleRemoveAreaCommand* ripple_remove_command_;

};

}

#endif // TIMELINEUNDOPOINTER_H
