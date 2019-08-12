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

#ifndef TIMELINEOUTPUT_H
#define TIMELINEOUTPUT_H

#include "node/block/block.h"
#include "panel/timeline/timeline.h"

/**
 * @brief Node that represents the end of the Timeline as well as a time traversal Node
 */
class TimelineOutput : public Block
{
  Q_OBJECT
public:
  TimelineOutput();

  virtual Type type() override;

  virtual QString Name() override;
  virtual QString id() override;
  virtual QString Category() override;
  virtual QString Description() override;

  void AttachTimeline(TimelinePanel* timeline);

  virtual rational length() override;

  virtual void Refresh() override;

public slots:
  virtual void Process(const rational &time) override;

private:
  /**
   * @brief Sets this Block as the only block in the Timeline (creating essentially a one clip sequence)
   */
  void ConnectBlockInternal(Block* block);

  /**
   * @brief Adds a Block to the parent graph so it can be connected to other Nodes
   *
   * Also runs through Node's dependencies (the Nodes whose outputs are connected to this Node's inputs)
   */
  void AddBlockToGraph(Block* block);

  QVector<Block*> block_cache_;

  Block* first_block();

  Block* attached_block();

  Block* first_block_;
  Block* current_block_;

  TimelinePanel* attached_timeline_;

private slots:
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
};

#endif // TIMELINEOUTPUT_H
