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
  track_type_(kTrackTypeNone),
  block_invalidate_cache_stack_(0),
  index_(-1)
{
  block_input_ = new NodeInputArray("block_in");
  AddParameter(block_input_);
  connect(block_input_, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SLOT(BlockConnected(NodeEdgePtr)));
  connect(block_input_, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SLOT(BlockDisconnected(NodeEdgePtr)));
  connect(block_input_, SIGNAL(SizeChanged(int)), this, SLOT(BlockListSizeChanged(int)));

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
  return kTrack;
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

const int &TrackOutput::Index()
{
  return index_;
}

void TrackOutput::SetIndex(const int &index)
{
  index_ = index;
}

TrackOutput *TrackOutput::next_track()
{
  return ValueToPtr<TrackOutput>(track_input_->get_realtime_value_of_connected_output());
}

NodeInput *TrackOutput::track_input()
{
  return track_input_;
}

NodeOutput* TrackOutput::track_output()
{
  return track_output_;
}

Block *TrackOutput::BlockContainingTime(const rational &time)
{
  foreach (Block* block, block_cache_) {
    if (block->in() < time && block->out() > time) {
      return block;
    } else if (block->out() == time) {
      break;
    }
  }

  return nullptr;
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
  /*// We intercept IC signals from Blocks since we may be performing several options and they may over-signal
  if (from == previous_input() && block_invalidate_cache_stack_ == 0) {
    Node::InvalidateCache(qMax(start_range, rational(0)), qMin(end_range, in()), from);
  } else {
  }*/
  Node::InvalidateCache(start_range, end_range, from);
}

QVariant TrackOutput::Value(NodeOutput *output)
{
  if (output == track_output_) {
    // Set track output correctly
    return PtrToValue(this);
  }

  // Run default node processing
  return Block::Value(output);
}

void TrackOutput::InsertBlockBefore(Block* block, Block* after)
{
  InsertBlockAtIndex(block, block_cache_.indexOf(after));
}

void TrackOutput::InsertBlockAfter(Block *block, Block *before)
{
  int before_index = block_cache_.indexOf(before);

  Q_ASSERT(before_index >= 0);

  if (before_index == block_cache_.size() - 1) {
    AppendBlock(block);
  } else {
    InsertBlockAtIndex(block, before_index + 1);
  }
}

void TrackOutput::PrependBlock(Block *block)
{
  InsertBlockAtIndex(block, 0);
}

void TrackOutput::InsertBlockAtIndex(Block *block, int index)
{
  AddBlockToGraph(block);

  block_input_->InsertAt(index);
  NodeParam::ConnectEdge(block->block_output(),
                         block_input_->ParamAt(index));
}

void TrackOutput::AppendBlock(Block *block)
{
  AddBlockToGraph(block);

  BlockInvalidateCache();

  int last_index = block_input_->GetSize();
  block_input_->Append();
  NodeParam::ConnectEdge(block->block_output(),
                         block_input_->ParamAt(last_index));

  UnblockInvalidateCache();

  // Invalidate area that block was added to
  InvalidateCache(block->in(), track_length());
}

void TrackOutput::AddBlockToGraph(Block *block)
{
  // Find the parent graph
  NodeGraph* graph = static_cast<NodeGraph*>(parent());
  graph->AddNodeWithDependencies(block);
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

  ReplaceBlock(block, gap);
}

void TrackOutput::RippleRemoveBlock(Block *block)
{
  BlockInvalidateCache();

  rational remove_in = block->in();

  int index_of_block_to_remove = block_cache_.indexOf(block);

  block_input_->RemoveAt(index_of_block_to_remove);

  UnblockInvalidateCache();

  InvalidateCache(remove_in, track_length());

  // FIXME: Should there be removing the Blocks from the graph?
}

void TrackOutput::ReplaceBlock(Block *old, Block *replace)
{
  Q_ASSERT(old->length() == replace->length());

  BlockInvalidateCache();

  AddBlockToGraph(replace);

  int index_of_old_block = block_cache_.indexOf(old);

  NodeParam::DisconnectEdge(old->block_output(),
                            block_input_->ParamAt(index_of_old_block));

  NodeParam::ConnectEdge(replace->block_output(),
                         block_input_->ParamAt(index_of_old_block));

  UnblockInvalidateCache();

  InvalidateCache(replace->in(), replace->out());
}

TrackOutput *TrackOutput::TrackFromBlock(Block *block)
{
  NodeOutput* output = block->block_output();

  foreach (NodeEdgePtr edge, output->edges()) {
    Node* n = edge->input()->parentNode();

    if (n->IsTrack()) {
      return static_cast<TrackOutput*>(n);
    }
  }

  return nullptr;
}

const rational &TrackOutput::track_length()
{
  return track_length_;
}

bool TrackOutput::IsTrack()
{
  return true;
}

void TrackOutput::UpdateInOutFrom(int index)
{
  Q_ASSERT(index >= 0);
  Q_ASSERT(index < block_cache_.size());

  rational new_track_length;

  for (int i=index;i<block_cache_.size();i++) {
    Block* b = block_cache_.at(i);

    if (b) {
      rational prev_out;

      // Find previous block and retrieve its out point (if there isn't one, this in will be set to 0)
      for (int j=i-1;j>=0;j--) {
        Block* previous = block_cache_.at(j);
        if (previous) {
          prev_out = previous->out();
          break;
        }
      }

      rational new_out = prev_out + b->length();

      b->set_in(prev_out);
      b->set_out(new_out);

      new_track_length = new_out;

      emit b->Refreshed();
    }
  }

  // Update track length
  if (new_track_length != track_length_) {
    track_length_ = new_track_length;
    emit TrackLengthChanged();
  }
}

void TrackOutput::UpdatePreviousAndNextOfIndex(int index)
{
  Block* ref = block_cache_.at(index);

  Block* previous = nullptr;
  Block* next = nullptr;

  // Find previous
  for (int i=index-1;i>=0;i--) {
    previous = block_cache_.at(i);

    if (previous) {
      break;
    }
  }

  // Find next
  for (int i=index+1;i<block_cache_.size();i++) {
    next = block_cache_.at(i);

    if (next) {
      break;
    }
  }

  if (ref) {
    // Link blocks together
    ref->set_previous(previous);
    ref->set_next(next);

    if (previous)
      previous->set_next(ref);

    if (next)
      next->set_previous(ref);
  } else {
    // Link previous and next together
    if (previous)
      previous->set_next(next);

    if (next)
      next->set_previous(previous);
  }
}

void TrackOutput::BlockConnected(NodeEdgePtr edge)
{
  int block_index = block_input_->IndexOfSubParameter(edge->input());

  Q_ASSERT(block_index >= 0);

  Node* connected_node = edge->output()->parentNode();
  Block* connected_block = connected_node->IsBlock() ? static_cast<Block*>(connected_node) : nullptr;
  block_cache_.replace(block_index, connected_block);
  UpdatePreviousAndNextOfIndex(block_index);
  UpdateInOutFrom(block_index);

  if (connected_block) {
    connect(connected_block, SIGNAL(LengthChanged(const rational&)), this, SLOT(BlockLengthChanged()));

    emit BlockAdded(connected_block);
  }
}

void TrackOutput::BlockDisconnected(NodeEdgePtr edge)
{
  int block_index = block_input_->IndexOfSubParameter(edge->input());

  Q_ASSERT(block_index >= 0);

  block_cache_.replace(block_index, nullptr);
  UpdatePreviousAndNextOfIndex(block_index);
  UpdateInOutFrom(block_index);

  Node* connected_node = edge->output()->parentNode();
  Block* connected_block = connected_node->IsBlock() ? static_cast<Block*>(connected_node) : nullptr;
  if (connected_block) {
    disconnect(connected_block, SIGNAL(LengthChanged(const rational&)), this, SLOT(BlockLengthChanged()));

    // Update previous and next references
    emit BlockRemoved(connected_block);
  }
}

void TrackOutput::BlockListSizeChanged(int size)
{
  int old_size = block_cache_.size();

  block_cache_.resize(size);

  if (size > old_size) {
    // Fill new slots with nullptr
    for (int i=old_size;i<size;i++) {
      block_cache_.replace(i, nullptr);
    }
  }
}

void TrackOutput::BlockLengthChanged()
{
  // Assumes sender is a Block
  Block* b = static_cast<Block*>(sender());

  int index = block_cache_.indexOf(b);

  Q_ASSERT(index >= 0);

  UpdateInOutFrom(index);
}
