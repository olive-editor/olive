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

#include "track.h"

#include <QDebug>

#include "node/block/gap/gap.h"
#include "node/graph.h"

TrackOutput::TrackOutput() :
  current_block_(this),
  track_type_(kTrackTypeNone),
  block_invalidate_cache_stack_(0),
  index_(-1)
{
  track_input_ = new NodeInput("track_in");
  track_input_->set_data_type(NodeParam::kTrack);
  track_input_->set_dependent(false);
  AddParameter(track_input_);

  track_output_ = new NodeOutput("track_out");
  AddParameter(track_output_);
}

void TrackOutput::set_track_type(const TrackType &track_type)
{
  track_type_ = track_type;
}

const TrackType& TrackOutput::track_type()
{
  return track_type_;
}

Block::Type TrackOutput::type()
{
  return kEnd;
}

Block *TrackOutput::copy()
{
  return new TrackOutput();
}

QString TrackOutput::Name()
{
  return tr("Track");
}

QString TrackOutput::id()
{
  return "org.olivevideoeditor.Olive.track";
}

QString TrackOutput::Category()
{
  return tr("Output");
}

QString TrackOutput::Description()
{
  return tr("Node for representing and processing a single array of Blocks sorted by time. Also represents the end of "
            "a Sequence.");
}

void TrackOutput::Refresh()
{
  QVector<Block*> detect_attached_blocks;

  Block* prev = previous();
  while (prev != nullptr) {
    detect_attached_blocks.prepend(prev);

    if (!block_cache_.contains(prev)) {
      emit BlockAdded(prev);
    }

    prev = prev->previous();
  }

  foreach (Block* b, block_cache_) {
    if (!detect_attached_blocks.contains(b)) {
      // If the current block was removed, stop referencing it
      if (current_block_ == b) {
        current_block_ = this;
      }

      emit BlockRemoved(b);
    }
  }

  block_cache_ = detect_attached_blocks;

  Block::Refresh();
}

const int &TrackOutput::Index()
{
  return index_;
}

void TrackOutput::SetIndex(const int &index)
{
  index_ = index;
}

QList<NodeDependency> TrackOutput::RunDependencies(NodeOutput* output, const rational &time)
{
  QList<NodeDependency> deps;

  if (output == buffer_output()) {
    ValidateCurrentBlock(time);

    if (current_block_ != this) {
      deps.append(NodeDependency(current_block_->buffer_output(), time, time));
    }
  }

  return deps;
}

TrackOutput *TrackOutput::next_track()
{
  return ValueToPtr<TrackOutput>(track_input_->get_value(0));
}

NodeInput *TrackOutput::track_input()
{
  return track_input_;
}

NodeOutput* TrackOutput::track_output()
{
  return track_output_;
}

Block *TrackOutput::NearestBlockBefore(const rational &time)
{
  foreach (Block* block, block_cache_) {
    // Blocks are sorted by time, so the first Block who's out point is at/after this time is the correct Block
    if (block->out() >= time) {
      return block;
    }
  }

  return this;
}

Block *TrackOutput::NearestBlockAfter(const rational &time)
{
  foreach (Block* block, block_cache_) {
    // Blocks are sorted by time, so the first Block after this time is the correct Block
    if (block->in() >= time) {
      return block;
    }
  }

  return this;
}

const QVector<Block *> &TrackOutput::Blocks()
{
  return block_cache_;
}

void TrackOutput::InvalidateCache(const rational &start_range, const rational &end_range, NodeInput *from)
{
  // We intercept IC signals from Blocks since we may be performing several options and they may over-signal
  if (from == previous_input() && block_invalidate_cache_stack_ == 0) {
    Node::InvalidateCache(qMax(start_range, rational(0)), qMin(end_range, in()), from);
  } else {
    Node::InvalidateCache(start_range, end_range, from);
  }
}

QVariant TrackOutput::Value(NodeOutput *output, const rational &in, const rational &out)
{
  if (output == track_output_) {
    // Set track output correctly
    return PtrToValue(this);
  } else if (output == buffer_output()) {
    ValidateCurrentBlock(in);

    if (current_block_ != this) {
      // At this point, we must have found the correct block so we use its texture output to produce the image
      return current_block_->buffer_output()->get_value(in, out);
    }

    // No texture is valid
    return 0;
  }

  // Run default node processing
  return Block::Value(output, in, out);
}

void TrackOutput::InsertBlockBetweenBlocks(Block *block, Block *before, Block *after)
{
  AddBlockToGraph(block);

  Block::DisconnectBlocks(before, after);
  Block::ConnectBlocks(before, block);
  Block::ConnectBlocks(block, after);
}

void TrackOutput::InsertBlockBefore(Block* block, Block* after)
{
  Block* before = after->previous();

  // If a block precedes this one, just insert between them
  if (before != nullptr) {
    InsertBlockBetweenBlocks(block, before, after);
  } else {
    AddBlockToGraph(block);

    // Otherwise, just connect the block since there's no before clip to insert between
    Block::ConnectBlocks(block, after);
  }
}

void TrackOutput::InsertBlockAfter(Block *block, Block *before)
{
  InsertBlockBetweenBlocks(block, before, before->next());
}

void TrackOutput::PrependBlock(Block *block)
{
  AddBlockToGraph(block);

  if (block_cache_.isEmpty()) {
    ConnectBlockInternal(block);
  } else {
    Block::ConnectBlocks(block, block_cache_.first());
  }
}

void TrackOutput::InsertBlockAtIndex(Block *block, int index)
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

void TrackOutput::AppendBlock(Block *block)
{
  AddBlockToGraph(block);

  BlockInvalidateCache();

  if (block_cache_.isEmpty()) {
    ConnectBlockInternal(block);
  } else {
    InsertBlockBetweenBlocks(block, block_cache_.last(), this);
  }

  UnblockInvalidateCache();

  // Invalidate area that block was added to
  InvalidateCache(block->in(), in());
}

void TrackOutput::ConnectBlockInternal(Block *block)
{
  AddBlockToGraph(block);

  Block::ConnectBlocks(block, this);
}

void TrackOutput::AddBlockToGraph(Block *block)
{
  // Find the parent graph
  NodeGraph* graph = static_cast<NodeGraph*>(parent());
  graph->AddNodeWithDependencies(block);
}

void TrackOutput::ValidateCurrentBlock(const rational &time)
{
  // This node representso the end of the timeline, so being beyond its in point is considered the end of the sequence
  if (time >= in()) {
    current_block_ = this;
    return;
  }

  // If we're here, we need to find the current clip to display

  // If the time requested is an earlier Block, traverse earlier until we find it
  while (time < current_block_->in()) {
    current_block_ = current_block_->previous();
  }

  // If the time requested is in a later Block, traverse later
  while (time >= current_block_->out()) {
    current_block_ = current_block_->next();
  }
}

void TrackOutput::BlockInvalidateCache()
{
  block_invalidate_cache_stack_++;
}

void TrackOutput::UnblockInvalidateCache()
{
  block_invalidate_cache_stack_--;
}

void TrackOutput::RemoveBlock(Block *block)
{
  GapBlock* gap = new GapBlock();
  gap->set_length(block->length());

  Block* previous = block->previous();
  Block* next = block->next();

  // Remove block
  RippleRemoveBlock(block);

  if (previous == nullptr) {
    // Block must be at the beginning
    PrependBlock(gap);
  } else {
    InsertBlockBetweenBlocks(gap, previous, next);
  }
}

void TrackOutput::RippleRemoveBlock(Block *block)
{
  BlockInvalidateCache();

  rational remove_in = block->in();

  Block* previous = block->previous();
  Block* next = block->next();

  if (previous != nullptr) {
    Block::DisconnectBlocks(previous, block);
  }

  if (next != nullptr) {
    Block::DisconnectBlocks(block, next);
  }

  if (previous != nullptr && next != nullptr) {
    Block::ConnectBlocks(previous, next);
  }

  UnblockInvalidateCache();

  InvalidateCache(remove_in, in());

  // FIXME: Should there be removing the Blocks from the graph?
}

void TrackOutput::ReplaceBlock(Block *old, Block *replace)
{
  Q_ASSERT(old->length() == replace->length());

  Block* previous = old->previous();
  Block* next = old->next();

  BlockInvalidateCache();

  AddBlockToGraph(replace);

  // Disconnect old block from its surroundings
  if (previous != nullptr) {
    Block::DisconnectBlocks(previous, old);
    Block::ConnectBlocks(previous, replace);
  }

  if (next != nullptr) {
    Block::DisconnectBlocks(old, next);
    Block::ConnectBlocks(replace, next);
  }

  UnblockInvalidateCache();

  InvalidateCache(replace->in(), replace->out());
}

TrackOutput *TrackOutput::TrackFromBlock(Block *block)
{
  Block* n = block;

  // Find last valid block in Sequence and assume its a track
  while (n->next() != nullptr) {
    n = n->next();
  }

  // Downside of this approach is the usage of dynamic_cast, alternative would be looping through all known tracks and
  // seeing if the contain the Block, but this seems slower
  return dynamic_cast<TrackOutput*>(n);
}
