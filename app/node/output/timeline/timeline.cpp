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

#include "timeline.h"

#include <QDebug>

#include "node/block/gap/gap.h"
#include "node/graph.h"

TimelineOutput::TimelineOutput() :
  first_block_(nullptr),
  current_block_(this),
  attached_timeline_(nullptr)
{
}

Block::Type TimelineOutput::type()
{
  return kEnd;
}

QString TimelineOutput::Name()
{
  return tr("Timeline");
}

QString TimelineOutput::id()
{
  return "org.olivevideoeditor.Olive.timeline";
}

QString TimelineOutput::Category()
{
  return tr("Output");
}

QString TimelineOutput::Description()
{
  return tr("Node for communicating between a Timeline panel and the node graph. Also represents the end of a"
            "Sequence.");
}

void TimelineOutput::AttachTimeline(TimelinePanel *timeline)
{
  if (attached_timeline_ != nullptr) {
    disconnect(attached_timeline_, SIGNAL(RequestInsertBlockAtIndex(Block*, int)), this, SLOT(InsertBlockAtIndex(Block*, int)));
    disconnect(attached_timeline_, SIGNAL(RequestPlaceBlock(Block*, rational)), this, SLOT(PlaceBlock(Block*, rational)));

    attached_timeline_->Clear();
  }

  attached_timeline_ = timeline;

  if (attached_timeline_ != nullptr) {
    attached_timeline_->Clear();

    // FIXME: TEST CODE ONLY
    attached_timeline_->SetTimebase(rational(1001, 30000));
    // END TEST CODE

    Block* previous_block = attached_block();
    while (previous_block != nullptr) {

      // FIXME: Dynamic cast is a dumb way of doing this
      ClipBlock* clip_block = dynamic_cast<ClipBlock*>(previous_block);

      if (clip_block != nullptr) {
        attached_timeline_->AddClip(clip_block);
      }

      previous_block = previous_block->previous();
    }

    connect(attached_timeline_, SIGNAL(RequestInsertBlockAtIndex(Block*, int)), this, SLOT(InsertBlockAtIndex(Block*, int)));
    connect(attached_timeline_, SIGNAL(RequestPlaceBlock(Block*, rational)), this, SLOT(PlaceBlock(Block*, rational)));
  }
}

rational TimelineOutput::length()
{
  return 0;
}

void TimelineOutput::Refresh()
{
  QVector<Block*> detect_attached_blocks;

  Block* previous = attached_block();
  while (previous != nullptr) {
    detect_attached_blocks.prepend(previous);

    if (attached_timeline_ != nullptr && !block_cache_.contains(previous)) {
      // FIXME: Remove the need to cast this
      attached_timeline_->AddClip(static_cast<ClipBlock*>(previous));
    }

    previous = previous->previous();
  }

  if (attached_timeline_ != nullptr) {
    foreach (Block* b, block_cache_) {
      if (!detect_attached_blocks.contains(b)) {
        attached_timeline_->RemoveClip(static_cast<ClipBlock*>(b));
      }
    }
  }

  block_cache_ = detect_attached_blocks;

  Block::Refresh();
}

void TimelineOutput::Process(const rational &time)
{
  // Run default node processing
  Block::Process(time);

  // This node representso the end of the timeline, so being beyond its in point is considered the end of the sequence
  if (time >= in()) {
    texture_output()->set_value(0);
    current_block_ = this;
    return;
  }

  // If we're here, we need to find the current clip to display
  // attached_block() is guaranteed to not be nullptr if we didn't return before
  current_block_ = attached_block();

  // If the time requested is an earlier Block, traverse earlier until we find it
  while (time < current_block_->in()) {
    current_block_ = current_block_->previous();
  }

  // If the time requested is in a later Block, traverse later
  while (time >= current_block_->out()) {
    current_block_ = current_block_->next();
  }

  // At this point, we must have found the correct block so we use its texture output to produce the image
  texture_output()->set_value(current_block_->texture_output()->get_value(time));
}

void TimelineOutput::InsertBlockBetweenBlocks(Block *block, Block *before, Block *after)
{
  AddBlockToGraph(block);

  Block::DisconnectBlocks(before, after);
  Block::ConnectBlocks(before, block);
  Block::ConnectBlocks(block, after);
}

Block *TimelineOutput::first_block()
{
  if (block_cache_.isEmpty()) {
    return nullptr;
  }

  return block_cache_.first();
}

Block *TimelineOutput::attached_block()
{
  return ValueToPtr<Block>(previous_input()->get_value(0));
}

void TimelineOutput::PrependBlock(Block *block)
{
  AddBlockToGraph(block);

  if (block_cache_.isEmpty()) {
    ConnectBlockInternal(block);
  } else {
    Block::ConnectBlocks(block, block_cache_.first());
  }
}

void TimelineOutput::InsertBlockAtIndex(Block *block, int index)
{
  AddBlockToGraph(block);

  if (block_cache_.isEmpty()) {

    // If there are no blocks connected, the index doesn't matter. Just connect it.
    ConnectBlockInternal(block);

  } else if (index == 0) {

    // If the index is 0, it goes at the very beginning
    PrependBlock(block);

  } else if (index >= block_cache_.size()) {

    // Append Block at the end
    AppendBlock(block);

  } else {

    // Insert Block just before the Block currently at that index so that it becomes the new Block at that index
    InsertBlockBetweenBlocks(block, block_cache_.at(index - 1), block_cache_.at(index));

  }
}

void TimelineOutput::AppendBlock(Block *block)
{
  AddBlockToGraph(block);

  if (block_cache_.isEmpty()) {
    ConnectBlockInternal(block);
  } else {
    InsertBlockBetweenBlocks(block, block_cache_.last(), this);
  }
}

void TimelineOutput::ConnectBlockInternal(Block *block)
{
  AddBlockToGraph(block);

  Block::ConnectBlocks(block, this);
}

void TimelineOutput::AddBlockToGraph(Block *block)
{
  // Find the parent graph
  NodeGraph* graph = static_cast<NodeGraph*>(parent());
  graph->AddNodeWithDependencies(block);
}

void TimelineOutput::PlaceBlock(Block *block, rational start)
{
  AddBlockToGraph(block);

  if (start == 0) {
    // FIXME: Remove existing

    PrependBlock(block);
    return;
  }

  // Check if the placement location is past the end of the timeline
  if (start > in()) {
    // FIXME: Remove existing

    // If so, insert a gap here
    GapBlock* gap = new GapBlock();
    gap->set_length(start - in());

    // Then append them
    AppendBlock(gap);
    AppendBlock(block);

    return;
  }

  // Check if the Block is placed at the in point of an existing Block, in which case a simple insert between will
  // suffice
  for (int i=1;i<block_cache_.size();i++) {
    Block* comparison = block_cache_.at(i);

    if (comparison->in() == start) {
      Block* previous = block_cache_.at(i-1);

      // InsertBlockAtIndex() could work here, but this function is faster since we've already found the Blocks
      InsertBlockBetweenBlocks(block, previous, comparison);
      return;
    }
  }
}
