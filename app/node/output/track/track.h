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
#include "timeline/tracktypes.h"

/**
 * @brief A time traversal Node for sorting through one channel/track of Blocks
 */
class TrackOutput : public Block
{
  Q_OBJECT
public:
  TrackOutput();

  const TrackType& track_type();
  void set_track_type(const TrackType& track_type);

  virtual Type type() override;

  virtual Block* copy() override;

  virtual QString Name() override;
  virtual QString id() override;
  virtual QString Category() override;
  virtual QString Description() override;

  const int& Index();
  void SetIndex(const int& index);

  TrackOutput* next_track();

  NodeInput* track_input();

  NodeOutput* track_output();

  Block* BlockContainingTime(const rational& time);

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
   * @brief Removes a Block and places a Gap in its place
   */
  void RemoveBlock(Block* block);

  /**
   * @brief Removes a Block pushing all subsequent Blocks earlier to take up the space
   */
  void RippleRemoveBlock(Block* block);

  /**
   * @brief Replaces Block `old` with Block `replace`
   *
   * Both blocks must have equal lengths.
   */
  void ReplaceBlock(Block* old, Block* replace);

  void BlockInvalidateCache();

  void UnblockInvalidateCache();

  /**
   * @brief Adds a Block to the parent graph so it can be connected to other Nodes
   *
   * Also runs through Node's dependencies (the Nodes whose outputs are connected to this Node's inputs)
   */
  void AddBlockToGraph(Block* block);

  static TrackOutput* TrackFromBlock(Block* block);

  const rational& track_length();

  virtual bool IsTrack() override;

signals:
  /**
   * @brief Signal emitted when a Block is added to this Track
   */
  void BlockAdded(Block* block);

  /**
   * @brief Signal emitted when a Block is removed from this Track
   */
  void BlockRemoved(Block* block);

  /**
   * @brief Signal emitted when the length of the track has changed
   */
  void TrackLengthChanged();

protected:
  virtual QVariant Value(NodeOutput* output) override;

private:
  void UpdateInOutFrom(int index);

  void UpdatePreviousAndNextOfIndex(int index);

  QVector<Block*> block_cache_;

  NodeInputArray* block_input_;

  NodeInput* track_input_;

  NodeOutput* track_output_;

  TrackType track_type_;

  rational track_length_;

  int block_invalidate_cache_stack_;

  int index_;

private slots:
  void BlockConnected(NodeEdgePtr edge);

  void BlockDisconnected(NodeEdgePtr edge);

  void BlockListSizeChanged(int size);

  void BlockLengthChanged();

};

#endif // TRACKOUTPUT_H
