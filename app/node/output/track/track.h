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

#ifndef TRACKOUTPUT_H
#define TRACKOUTPUT_H

#include "node/block/block.h"

/**
 * @brief A time traversal Node for sorting through one channel/track of Blocks
 */
class TrackOutput : public Block
{
  Q_OBJECT
public:
  TrackOutput();

  virtual Type type() override;

  virtual Block* copy() override;

  virtual QString Name() override;
  virtual QString id() override;
  virtual QString Category() override;
  virtual QString Description() override;

  virtual void set_length(const rational &length) override;

  virtual void Refresh() override;

  const int& Index();
  void SetIndex(const int& index);

  /**
   * @brief Override swaps "attached block" with "current block"
   */
  virtual QList<NodeDependency> RunDependencies(NodeOutput* param, const rational& time) override;

  TrackOutput* next_track();

  NodeInput* track_input();

  NodeOutput* track_output();

  Block* NearestBlockBefore(const rational& time);

  Block* NearestBlockAfter(const rational& time);

  const QVector<Block*>& Blocks();

  virtual void InvalidateCache(const rational& start_range, const rational& end_range, NodeInput* from = nullptr) override;

  /**
   * @brief Adds Block `block` at the very beginning of the Sequence before all other clips
   */
  void PrependBlock(Block* block);

  /**
   * @brief Inserts Block `block` at a specific index (0 is the start of the timeline)
   *
   * If the index == 0, this function does the same as PrependBlock(). If the index >= the current number of blocks,
   * this function is the same as AppendBlock().
   */
  void InsertBlockAtIndex(Block* block, int index);

  /**
   * @brief Inserts a Block between two other Blocks
   *
   * Disconnects `before` and `after`, and connects them to `block` with `block` in between.
   */
  void InsertBlockBetweenBlocks(Block* block, Block* before, Block* after);

  /**
   * @brief Inserts Block after another Block
   *
   * Equivalent to calling InsertBlockBetweenBlocks(block, before, before->next())
   */
  void InsertBlockAfter(Block* block, Block* before);

  /**
   * @brief Inserts Block before another Block
   */
  void InsertBlockBefore(Block* block, Block* after);

  /**
   * @brief Adds Block `block` at the very end of the Sequence after all other clips
   */
  void AppendBlock(Block* block);

  /**
   * @brief Destructively places `block` at the in point `start`
   *
   * The Block is guaranteed to be placed at the starting point specified. If there are Blocks in this area, they are
   * either trimmed or removed to make space for this Block. Additionally, if the Block is placed beyond the end of
   * the Sequence, a GapBlock is inserted to compensate.
   */
  void PlaceBlock(Block* block, rational start);

  /**
   * @brief Removes a Block and places a Gap in its place
   */
  void RemoveBlock(Block* block);

  /**
   * @brief Removes a Block pushing all subsequent Blocks earlier to take up the space
   */
  void RippleRemoveBlock(Block* block);

  /**
   * @brief Splits `block` into two Blocks at the Sequence point `time`
   *
   * @return
   *
   * The second block created as a result of this split
   */
  Block *SplitBlock(Block* block, rational time);

  /**
   * @brief Attempt to split at a certain time
   *
   * Finds the Block that surrounds this time and splits it. If there is no Block there, this is a no-op.
   */
  void SplitAtTime(rational time);

  /**
   * @brief Clears the area between in and out
   *
   * The area between `in` and `out` is guaranteed to be freed. BLocks are trimmed and removed to free this space.
   * By default, nothing takes this area meaning all subsequent clips are pushed backward, however you can specify
   * a block to insert at the `in` point. No checking is done to ensure `insert` is the same length as `in` to `out`.
   */
  void RippleRemoveArea(rational in, rational out, Block* insert = nullptr);

  /**
   * @brief Replaces Block `old` with Block `replace`
   *
   * Both blocks must have equal lengths.
   */
  void ReplaceBlock(Block* old, Block* replace);

signals:
  /**
   * @brief Signal emitted when a Block is added to this Track
   */
  void BlockAdded(Block* block);

  /**
   * @brief Signal emitted when a Block is removed from this Track
   */
  void BlockRemoved(Block* block);

protected:
  virtual QVariant Value(NodeOutput* output, const rational& time) override;

private:
  /**
   * @brief Sets this Block as the only block in the Timeline (creating essentially a one clip sequence)
   */
  void ConnectBlockInternal(Block* block);

  /**
   * @brief Disconnects t
   */
  void RemoveBlockInternal();

  /**
   * @brief Adds a Block to the parent graph so it can be connected to other Nodes
   *
   * Also runs through Node's dependencies (the Nodes whose outputs are connected to this Node's inputs)
   */
  void AddBlockToGraph(Block* block);

  /**
   * @brief Sets current_block_ to the correct attached Block based on `time`
   */
  void ValidateCurrentBlock(const rational& time);

  void BlockInvalidateCache();
  void UnblockInvalidateCache();

  QVector<Block*> block_cache_;

  Block* current_block_;

  NodeInput* track_input_;

  NodeOutput* track_output_;

  int block_invalidate_cache_stack_;

  int index_;

private slots:

};

#endif // TRACKOUTPUT_H
