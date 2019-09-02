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
  current_block_(this)
{
  track_input_ = new NodeInput("track_in");
  track_input_->add_data_input(NodeParam::kTrack);
  AddParameter(track_input_);

  track_output_ = new NodeOutput("track_out");
  track_output_->set_data_type(NodeParam::kTrack);
  AddParameter(track_output_);
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

void TrackOutput::set_length(const rational &)
{
  // Prevent length changing on this Block
}

void TrackOutput::Refresh()
{
  QVector<Block*> detect_attached_blocks;

  Block* previous = attached_block();
  while (previous != nullptr) {
    detect_attached_blocks.prepend(previous);

    if (!block_cache_.contains(previous)) {
      emit BlockAdded(previous);
    }

    previous = previous->previous();
  }

  foreach (Block* b, block_cache_) {
    if (!detect_attached_blocks.contains(b)) {
      emit BlockRemoved(b);
    }
  }

  block_cache_ = detect_attached_blocks;

  Block::Refresh();
}

QList<NodeDependency> TrackOutput::RunDependencies(NodeOutput* output, const rational &time)
{
  QList<NodeDependency> deps;

  if (output == texture_output()) {
    ValidateCurrentBlock(time);

    if (current_block_ != this) {
      deps.append(NodeDependency(current_block_->texture_output(), time));
    }
  }

  return deps;
}

void TrackOutput::GenerateBlockWidgets()
{
  foreach (Block* block, block_cache_) {
    emit BlockAdded(block);
  }
}

void TrackOutput::DestroyBlockWidgets()
{
  foreach (Block* block, block_cache_) {
    emit BlockRemoved(block);
  }
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

#include "render/rendertexture.h"

QVariant TrackOutput::Value(NodeOutput *output, const rational &time)
{
  if (output == track_output_) {
    // Set track output correctly
    return PtrToValue(this);
  } else if (output == texture_output()) {
    ValidateCurrentBlock(time);

    if (current_block_ != this) {
      // At this point, we must have found the correct block so we use its texture output to produce the image
      return current_block_->texture_output()->get_value(time);
    }

    // No texture is valid
    return 0;
  }

  // Run default node processing
  return Block::Value(output, time);
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

Block *TrackOutput::attached_block()
{
  return ValueToPtr<Block>(previous_input()->get_value(0));
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

  if (block_cache_.isEmpty()) {
    ConnectBlockInternal(block);
  } else {
    InsertBlockBetweenBlocks(block, block_cache_.last(), this);
  }
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

void TrackOutput::PlaceBlock(Block *block, rational start)
{
  if (block_cache_.contains(block) && block->in() == start) {
    return;
  }

  AddBlockToGraph(block);

  // Check if the placement location is past the end of the timeline
  if (start >= in()) {
    GapBlock* gap = nullptr;

    if (start > in()) {
      // If so, insert a gap here
      gap = new GapBlock();
      gap->set_length(start - in());
    }

    InsertBlockBefore(block, this);

    if (gap != nullptr) {
      // Insert gap if we made one before
      InsertBlockBefore(gap, block);
    }
    return;
  }

  // Place the Block at this point
  RippleRemoveArea(start, start + block->length(), block);
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

  // FIXME: Should there be removing the Blocks from the graph?
}

Block* TrackOutput::SplitBlock(Block *block, rational time)
{
  if (time <= block->in() || time >= block->out()) {
    return nullptr;
  }

  rational original_length = block->length();

  block->set_length(time - block->in());

  Block* copy = block->copy();
  copy->set_length(original_length - block->length());
  InsertBlockAfter(copy, block);

  return copy;
}

void TrackOutput::SplitAtTime(rational time)
{
  // Find Block that contains this time
  for (int i=0;i<block_cache_.size();i++) {
    Block* b = block_cache_.at(i);

    if (b->out() == time) {
      // This time is between blocks, no split needs to occur
      return;
    } else if (b->in() < time && b->out() > time) {
      // We found the Block, split it
      SplitBlock(b, time);
      return;
    }
  }
}

void TrackOutput::RippleRemoveArea(rational in, rational out, Block *insert)
{
  // Block that needs to be split to remove this area
  Block* splice = nullptr;

  // Block whose out point exceeds `in` and needs to be trimmed
  Block* trim_out_to_in = nullptr;

  // Block whose in point exceeds `out` and needs to be trimmed
  Block* trim_in_to_out = nullptr;

  // Blocks that are entirely within the area and need removing
  QList<Block*> remove;

  // Iterate through blocks determining which need trimming/removing/splitting
  foreach (Block* block, block_cache_) {
    if (block->in() < in && block->out() > out) {
      // The area entirely within this Block
      splice = block;

      // We don't need to do anything else here
      break;
    } else if (block->in() >= in && block->out() <= out) {
      // This Block's is entirely within the area
      remove.append(block);
    } else if (block->in() < in && block->out() >= in) {
      // This Block's out point exceeds `in`
      trim_out_to_in = block;
    } else if (block->in() <= out && block->out() > out) {
      // This Block's in point exceeds `out`
      trim_in_to_out = block;
    }
  }

  // If we picked up a block to splice
  if (splice != nullptr) {

    // Split the block here
    Block* copy = SplitBlock(splice, in);

    // Perform all further actions as if we were just trimming these clips
    trim_out_to_in = splice;
    trim_in_to_out = copy;

  }

  // If we picked up a block to trim the in point of
  if (trim_in_to_out != nullptr && trim_in_to_out->in() < out) {
    rational new_length = trim_in_to_out->out() - out;

    // Push media_in forward to compensate
    rational length_diff = trim_in_to_out->length() - new_length;
    trim_in_to_out->set_media_in(trim_in_to_out->media_in() - length_diff);

    trim_in_to_out->set_length(new_length);
  }

  // Remove all blocks that are flagged for removal
  foreach (Block* remove_block, remove) {
    RippleRemoveBlock(remove_block);
  }

  // If we picked up a block to trim the out point of
  if (trim_out_to_in != nullptr && trim_out_to_in->out() > in) {
    trim_out_to_in->set_length(in - trim_out_to_in->in());
  }

  // If we were given a block to insert, insert it here
  if (insert != nullptr) {
    if (trim_out_to_in == nullptr) {
      // This is the start of the Sequence
      PrependBlock(insert);
    } else if (trim_in_to_out == nullptr) {
      // This is the end of the Sequence
      AppendBlock(insert);
    } else {
      // This is somewhere in the middle of the Sequence
      InsertBlockBetweenBlocks(insert, trim_out_to_in, trim_in_to_out);
    }
  }
}

void TrackOutput::ReplaceBlock(Block *old, Block *replace)
{
  Block* previous = old->previous();
  Block* next = old->next();

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
}
