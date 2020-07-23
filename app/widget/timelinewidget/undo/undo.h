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

#ifndef TIMELINEUNDOABLE_H
#define TIMELINEUNDOABLE_H

#include <QUndoCommand>

#include "node/block/block.h"
#include "node/block/gap/gap.h"
#include "node/block/transition/transition.h"
#include "node/output/track/track.h"
#include "node/output/track/tracklist.h"
#include "timeline/timelinepoints.h"
#include "undo/undocommand.h"

OLIVE_NAMESPACE_ENTER

class BlockResizeCommand : public UndoCommand {
public:
  BlockResizeCommand(Block* block, rational new_length, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  Block* block_;
  rational old_length_;
  rational new_length_;
};

class BlockResizeWithMediaInCommand : public UndoCommand {
public:
  BlockResizeWithMediaInCommand(Block* block, rational new_length, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  Block* block_;
  rational old_length_;
  rational new_length_;
};

class BlockTrimCommand : public UndoCommand {
public:
  BlockTrimCommand(TrackOutput *track, Block* block, rational new_length, Timeline::MovementMode mode, QUndoCommand* command = nullptr);

  virtual Project* GetRelevantProject() const override;

  void SetTrimIsARollEdit(bool e)
  {
    trim_is_a_roll_edit_ = e;
  }

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  TrackOutput* track_;
  Block* block_;
  rational old_length_;
  rational new_length_;
  Timeline::MovementMode mode_;

  Block* adjacent_;
  bool we_created_adjacent_;
  bool we_deleted_adjacent_;

  bool trim_is_a_roll_edit_;

  QObject memory_manager_;

};

class BlockSetMediaInCommand : public UndoCommand {
public:
  BlockSetMediaInCommand(Block* block, rational new_media_in, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  Block* block_;
  rational old_media_in_;
  rational new_media_in_;
};

class BlockSetSpeedCommand : public UndoCommand {
public:
  BlockSetSpeedCommand(Block* block, const rational& new_speed, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  Block* block_;

  rational old_speed_;
  rational new_speed_;
};

class TrackRippleRemoveBlockCommand : public UndoCommand {
public:
  TrackRippleRemoveBlockCommand(TrackOutput* track, Block* block, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  TrackOutput* track_;

  Block* block_;

  Block* before_;
};

class TrackPrependBlockCommand : public UndoCommand {
public:
  TrackPrependBlockCommand(TrackOutput* track, Block* block, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  TrackOutput* track_;
  Block* block_;
};

class TrackInsertBlockAfterCommand : public UndoCommand {
public:
  TrackInsertBlockAfterCommand(TrackOutput* track, Block* block, Block* before, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  TrackOutput* track_;

  Block* block_;

  Block* before_;
};

/**
 * @brief Clears the area between in and out
 *
 * The area between `in` and `out` is guaranteed to be freed. BLocks are trimmed and removed to free this space.
 * By default, nothing takes this area meaning all subsequent clips are pushed backward, however you can specify
 * a block to insert at the `in` point. No checking is done to ensure `insert` is the same length as `in` to `out`.
 */
class TrackRippleRemoveAreaCommand : public UndoCommand {
public:
  TrackRippleRemoveAreaCommand(TrackOutput* track, rational in, rational out, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

  void SetInsert(Block* insert);

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

protected:
  Project* project_;

  TrackOutput* track_;
  rational in_;
  rational out_;

  bool splice_;

  Block* trim_out_;
  Block* trim_in_;
  QVector<Block*> removed_blocks_;

  rational trim_in_old_length_;
  rational trim_out_old_length_;

  rational trim_in_new_length_;
  rational trim_out_new_length_;

  Block* insert_;

  QObject memory_manager_;

  QList<UndoCommand*> remove_block_commands_;

};

class TrackListRippleRemoveAreaCommand : public UndoCommand {
public:
  TrackListRippleRemoveAreaCommand(TrackList* list, rational in, rational out, QUndoCommand* parent = nullptr);

  virtual ~TrackListRippleRemoveAreaCommand() override;

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  TrackList* list_;

  QList<TrackOutput*> working_tracks_;

  rational in_;

  rational out_;

  bool all_tracks_unlocked_;

  QVector<TrackRippleRemoveAreaCommand*> commands_;

};

class TimelineRippleRemoveAreaCommand : public UndoCommand {
public:
  TimelineRippleRemoveAreaCommand(ViewerOutput* timeline, rational in, rational out, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

private:
  ViewerOutput* timeline_;

};

class TrackListRippleToolCommand : public UndoCommand {
public:
  struct RippleInfo {
    Block* block;
    Block* ref_block;
    TrackOutput* track;
    rational new_length;
    rational old_length;
  };

  TrackListRippleToolCommand(TrackList* track_list,
                             const QList<RippleInfo>& info,
                             const Timeline::MovementMode& movement_mode,
                             QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  TrackList* track_list_;

  QList<RippleInfo> info_;
  Timeline::MovementMode movement_mode_;

  struct WorkingData {
    GapBlock* created_gap;
    Block* removed_gap_after;
  };

  QVector<WorkingData> working_data_;

  QObject memory_manager_;

  bool all_tracks_unlocked_;

};

/**
 * @brief Destructively places `block` at the in point `start`
 *
 * The Block is guaranteed to be placed at the starting point specified. If there are Blocks in this area, they are
 * either trimmed or removed to make space for this Block. Additionally, if the Block is placed beyond the end of
 * the Sequence, a GapBlock is inserted to compensate.
 */
class TrackPlaceBlockCommand : public TrackRippleRemoveAreaCommand {
public:
  TrackPlaceBlockCommand(TrackList *timeline, int track, Block* block, rational in, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  TrackList* timeline_;
  int track_index_;
  bool append_;
  GapBlock* gap_;
  int added_track_count_;
};

class BlockSplitCommand : public UndoCommand {
public:
  BlockSplitCommand(TrackOutput* track, Block* block, rational point, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

  Block* new_block();

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  TrackOutput* track_;
  Block* block_;

  rational new_length_;
  rational old_length_;
  rational point_;

  Block* new_block_;

  QList<NodeInput*> transitions_to_move_;

  QObject memory_manager_;

};

class TrackSplitAtTimeCommand : public UndoCommand {
public:
  TrackSplitAtTimeCommand(TrackOutput* track, rational point, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

private:
  TrackOutput* track_;

};

class BlockSplitPreservingLinksCommand : public UndoCommand {
public:
  BlockSplitPreservingLinksCommand(const QVector<Block *> &blocks, const QList<rational>& times, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

private:
  QVector<Block *> blocks_;

  QList<rational> times_;
};

/**
 * @brief Replaces Block `old` with Block `replace`
 *
 * Both blocks must have equal lengths.
 */
class TrackReplaceBlockCommand : public UndoCommand {
public:
  TrackReplaceBlockCommand(TrackOutput* track, Block* old, Block* replace, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  TrackOutput* track_;
  Block* old_;
  Block* replace_;
};

class TrackReplaceBlockWithGapCommand : public UndoCommand {
public:
  TrackReplaceBlockWithGapCommand(TrackOutput* track, Block* block, QUndoCommand* command = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  TrackOutput* track_;
  Block* block_;

  bool we_created_gap_;
  GapBlock* gap_;
  GapBlock* merged_gap_;

  QObject memory_manager_;

};

class TimelineRippleDeleteGapsAtRegionsCommand : public UndoCommand {
public:
  TimelineRippleDeleteGapsAtRegionsCommand(ViewerOutput* vo, const TimeRangeList& regions, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  ViewerOutput* timeline_;
  TimeRangeList regions_;

  QList<UndoCommand*> commands_;

};

class WorkareaSetEnabledCommand : public UndoCommand {
public:
  WorkareaSetEnabledCommand(Project *project, TimelinePoints* points, bool enabled, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  Project* project_;

  TimelinePoints* points_;

  bool old_enabled_;

  bool new_enabled_;

};

class WorkareaSetRangeCommand : public UndoCommand {
public:
  WorkareaSetRangeCommand(Project *project, TimelinePoints* points, const TimeRange& range, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  Project* project_;

  TimelinePoints* points_;

  TimeRange old_range_;

  TimeRange new_range_;

};

class BlockLinkManyCommand : public UndoCommand {
public:
  BlockLinkManyCommand(const QList<Block*> blocks, bool link, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

private:
  QList<Block*> blocks_;

};

class BlockLinkCommand : public UndoCommand {
public:
  BlockLinkCommand(Block* a, Block* b, bool link, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  Block* a_;

  Block* b_;

  bool link_;

  bool done_;

};

class BlockUnlinkAllCommand : public UndoCommand {
public:
  BlockUnlinkAllCommand(Block* block, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  Block* block_;

  QVector<Block*> unlinked_;

};

class BlockEnableDisableCommand : public UndoCommand {
public:
  BlockEnableDisableCommand(Block* block, bool enabled, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  Block* block_;

  bool old_enabled_;

  bool new_enabled_;

};

class TrackSlideCommand : public UndoCommand {
public:
  TrackSlideCommand(TrackOutput* track, const QList<Block*>& moving_blocks, Block* in_adjacent, Block* out_adjacent, const rational& movement, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  void slide_internal(bool undo);

  TrackOutput* track_;
  QList<Block*> blocks_;
  rational movement_;

  bool we_created_in_adjacent_;
  Block* in_adjacent_;
  bool we_created_out_adjacent_;
  Block* out_adjacent_;

  QObject memory_manager_;

};

class TrackListInsertGaps : public UndoCommand {
public:
  TrackListInsertGaps(TrackList* track_list, const rational& point, const rational& length, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  TrackList* track_list_;

  rational point_;

  rational length_;

  QList<TrackOutput*> working_tracks_;

  bool all_tracks_unlocked_;

  QList<Block*> gaps_to_extend_;

  QList<GapBlock*> gaps_added_;

  BlockSplitPreservingLinksCommand* split_command_;

};

class TransitionRemoveCommand : public UndoCommand {
public:
  TransitionRemoveCommand(TrackOutput *track, TransitionBlock* block, QUndoCommand *parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  TrackOutput* track_;

  TransitionBlock* block_;

  Block* out_block_;
  Block* in_block_;

};

OLIVE_NAMESPACE_EXIT

#endif // TIMELINEUNDOABLE_H
