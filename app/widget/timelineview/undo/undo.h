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
#include "node/output/timeline/timeline.h"
#include "node/output/track/track.h"

class BlockResizeCommand : public QUndoCommand {
public:
  BlockResizeCommand(Block* block, rational new_length, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  Block* block_;
  rational old_length_;
  rational new_length_;
};

class BlockResizeWithMediaInCommand : public QUndoCommand {
public:
  BlockResizeWithMediaInCommand(Block* block, rational new_length, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  Block* block_;
  rational old_length_;
  rational new_length_;
};

class BlockSetMediaInCommand : public QUndoCommand {
public:
  BlockSetMediaInCommand(Block* block, rational new_media_in, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  Block* block_;
  rational old_media_in_;
  rational new_media_in_;
};

class TrackRippleRemoveBlockCommand : public QUndoCommand {
public:
  TrackRippleRemoveBlockCommand(TrackOutput* track, Block* block, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  TrackOutput* track_;

  Block* block_;

  Block* before_;
  Block* after_;
};

class TrackInsertBlockBetweenBlocksCommand : public QUndoCommand {
public:
  TrackInsertBlockBetweenBlocksCommand(TrackOutput* track, Block* block, Block* before, Block* after, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  TrackOutput* track_;

  Block* block_;

  Block* before_;
  Block* after_;
};

/**
 * @brief Clears the area between in and out
 *
 * The area between `in` and `out` is guaranteed to be freed. BLocks are trimmed and removed to free this space.
 * By default, nothing takes this area meaning all subsequent clips are pushed backward, however you can specify
 * a block to insert at the `in` point. No checking is done to ensure `insert` is the same length as `in` to `out`.
 */
class TrackRippleRemoveAreaCommand : public QUndoCommand {
public:
  TrackRippleRemoveAreaCommand(TrackOutput* track, rational in, rational out, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

protected:
  TrackOutput* track_;
  rational in_;
  rational out_;

  Block* splice_;

  Block* trim_out_;
  QVector<Block*> removed_blocks_;
  Block* trim_in_;

  rational trim_in_old_length_;
  rational trim_out_old_length_;

  rational trim_in_new_length_;
  rational trim_out_new_length_;

  Block* insert_;

  QObject memory_manager_;
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
  TrackPlaceBlockCommand(TimelineOutput *timeline, int track, Block* block, rational in, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  TimelineOutput* timeline_;
  int track_index_;
  bool append_;
  GapBlock* gap_;
  int added_track_count_;
};

class BlockSplitCommand : public QUndoCommand {
public:
  BlockSplitCommand(TrackOutput* track, Block* block, rational point, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  TrackOutput* track_;
  Block* block_;

  rational new_length_;
  rational old_length_;

  Block* new_block_;

  QObject memory_manager_;
};

class TrackSplitAtTimeCommand : public QUndoCommand {
public:
  TrackSplitAtTimeCommand(TrackOutput* track, rational point, QUndoCommand* parent = nullptr);
};

/**
 * @brief Replaces Block `old` with Block `replace`
 *
 * Both blocks must have equal lengths.
 */
class TrackReplaceBlockCommand : public QUndoCommand {
public:
  TrackReplaceBlockCommand(TrackOutput* track, Block* old, Block* replace, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  TrackOutput* track_;
  Block* old_;
  Block* replace_;
};

#endif // TIMELINEUNDOABLE_H
