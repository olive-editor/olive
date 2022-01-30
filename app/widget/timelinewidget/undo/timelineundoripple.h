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

#ifndef TIMELINEUNDORIPPLE_H
#define TIMELINEUNDORIPPLE_H

#include "node/block/gap/gap.h"
#include "node/output/track/track.h"
#include "node/output/track/tracklist.h"
#include "node/project/sequence/sequence.h"
#include "timelineundogeneral.h"
#include "timelineundosplit.h"
#include "timelineundotrack.h"

namespace olive {

/**
 * @brief Clears the area between in and out
 *
 * The area between `in` and `out` is guaranteed to be freed. BLocks are trimmed and removed to free this space.
 * By default, nothing takes this area meaning all subsequent clips are pushed backward, however you can specify
 * a block to insert at the `in` point. No checking is done to ensure `insert` is the same length as `in` to `out`.
 */
class TrackRippleRemoveAreaCommand : public UndoCommand
{
public:
  TrackRippleRemoveAreaCommand(Track* track, const TimeRange& range);

  virtual ~TrackRippleRemoveAreaCommand() override;

  virtual Project* GetRelevantProject() const override
  {
    return track_->project();
  }

  /**
   * @brief Block to insert after if you want to insert something between this ripple
   */
  Block* GetInsertionIndex() const
  {
    return insert_previous_;
  }

  Block* GetSplicedBlock() const
  {
    if (splice_split_command_) {
      return splice_split_command_->new_block();
    }

    return nullptr;
  }

protected:
  virtual void prepare() override;

  virtual void redo() override;

  virtual void undo() override;

private:
  struct TrimOperation {
    Block* block;
    rational old_length;
    rational new_length;
  };

  struct RemoveOperation {
    Block* block;
    Block* before;
  };

  Track* track_;
  TimeRange range_;

  TrimOperation trim_out_;
  QVector<RemoveOperation> removals_;
  TrimOperation trim_in_;
  Block* insert_previous_;

  BlockSplitCommand* splice_split_command_;
  QVector<UndoCommand*> remove_block_commands_;

};

class TrackListRippleRemoveAreaCommand : public UndoCommand
{
public:
  TrackListRippleRemoveAreaCommand(TrackList* list, rational in, rational out) :
    list_(list),
    range_(in, out)
  {
  }

  virtual ~TrackListRippleRemoveAreaCommand() override
  {
    qDeleteAll(commands_);
  }

  virtual Project* GetRelevantProject() const override
  {
    return list_->parent()->project();
  }

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  TrackList* list_;

  QList<Track*> working_tracks_;

  TimeRange range_;

  bool all_tracks_unlocked_;

  QVector<TrackRippleRemoveAreaCommand*> commands_;

};

class TimelineRippleRemoveAreaCommand : public MultiUndoCommand
{
public:
  TimelineRippleRemoveAreaCommand(Sequence* timeline, rational in, rational out);

  virtual Project* GetRelevantProject() const override
  {
    return timeline_->project();
  }

private:
  Sequence* timeline_;

};

class TrackListRippleToolCommand : public UndoCommand
{
public:
  struct RippleInfo {
    Block* block;
    bool append_gap;
  };

  TrackListRippleToolCommand(TrackList* track_list,
                             const QHash<Track*, RippleInfo>& info,
                             const rational& ripple_movement,
                             const Timeline::MovementMode& movement_mode);

  virtual Project* GetRelevantProject() const override
  {
    return track_list_->parent()->project();
  }

protected:
  virtual void redo() override
  {
    ripple(true);
  }

  virtual void undo() override
  {
    ripple(false);
  }

private:
  void ripple(bool redo);

  TrackList* track_list_;

  QHash<Track*, RippleInfo> info_;
  rational ripple_movement_;
  Timeline::MovementMode movement_mode_;

  struct WorkingData {
    GapBlock* created_gap = nullptr;
    Block* removed_gap_after;
    rational old_length;
    rational earliest_point_of_change;
  };

  QHash<Track*, WorkingData> working_data_;

  QObject memory_manager_;

  bool all_tracks_unlocked_;

};

class TimelineRippleDeleteGapsAtRegionsCommand : public UndoCommand
{
public:
  TimelineRippleDeleteGapsAtRegionsCommand(Sequence* vo, const QVector<QPair<Track*, TimeRange> >& regions) :
    timeline_(vo),
    regions_(regions)
  {
  }

  virtual ~TimelineRippleDeleteGapsAtRegionsCommand() override
  {
    qDeleteAll(commands_);
  }

  virtual Project* GetRelevantProject() const override
  {
    return timeline_->project();
  }

  bool HasCommands() const
  {
    return !commands_.isEmpty();
  }

protected:
  virtual void prepare() override;

  virtual void redo() override;

  virtual void undo() override;

private:
  Sequence* timeline_;
  QVector<QPair<Track*, TimeRange> > regions_;

  QVector<UndoCommand*> commands_;

  struct RemovalRequest {
    GapBlock *gap;
    TimeRange range;
  };

};

class TimelineShiftCacheCommand : public UndoCommand
{
public:
  TimelineShiftCacheCommand(Sequence* timeline, const rational &from, const rational &to) :
    timeline_(timeline),
    from_(from),
    to_(to)
  {}

  virtual Project* GetRelevantProject() const override
  {
    return timeline_->project();
  }

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  Sequence* timeline_;

  rational from_;

  rational to_;

};

}

#endif // TIMELINEUNDORIPPLE_H
