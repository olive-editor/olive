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

#include "block.h"

#include <QDebug>

Block::Block() :
  next_(nullptr)
{
  previous_input_ = new NodeInput("prev_block");
  previous_input_->add_data_input(NodeParam::kBlock);
  AddParameter(previous_input_);

  block_output_ = new NodeOutput("block_out");
  block_output_->set_data_type(NodeParam::kBlock);
  AddParameter(block_output_);

  texture_output_ = new NodeOutput("tex_out");
  texture_output_->set_data_type(NodeParam::kTexture);
  AddParameter(texture_output_);

  connect(this, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SLOT(EdgeAddedSlot(NodeEdgePtr)));
  connect(this, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SLOT(EdgeRemovedSlot(NodeEdgePtr)));
}

QString Block::Category()
{
  return tr("Block");
}

const rational &Block::in()
{
  return in_point_;
}

const rational &Block::out()
{
  return out_point_;
}

const rational& Block::length()
{
  return length_;
}

void Block::set_length(const rational &length)
{
  length_ = length;

  RefreshFollowing();
}

Block *Block::previous()
{
  return ValueToPtr<Block>(previous_input_->get_value(0));
}

Block *Block::next()
{
  return next_;
}

NodeInput *Block::previous_input()
{
  return previous_input_;
}

QVariant Block::Value(NodeOutput *output, const rational &time)
{
  Q_UNUSED(time)

  if (output == block_output_) {
    // Simply set the output value to a pointer to this Block
    return PtrToValue(this);
  }

  return 0;
}

void Block::EdgeAddedSlot(NodeEdgePtr edge)
{
  if (edge->input() == previous_input()) {
    static_cast<Block*>(edge->output()->parent())->next_ = this;

    // The blocks surrounding this one have changed, we need to Refresh()
    RefreshFollowing();
  }
}

void Block::EdgeRemovedSlot(NodeEdgePtr edge)
{
  if (edge->input() == previous_input()) {
    static_cast<Block*>(edge->output()->parent())->next_ = nullptr;

    // The blocks surrounding this one have changed, we need to Refresh()
    RefreshFollowing();
  }
}

void Block::Refresh()
{  
  // Set in point to the out point of the previous Node
  if (previous() != nullptr) {
    in_point_ = previous()->out();
  } else {
    in_point_ = 0;
  }

  // Update out point by adding this clip's length to the just calculated in point
  out_point_ = in_point_ + length();

  InvalidateCache(nullptr, in_point_, out_point_);

  emit Refreshed();
}

void Block::RefreshFollowing()
{
  Refresh();

  Block* next_block = next();

  while (next_block != nullptr) {
    next_block->Refresh();

    next_block = next_block->next();
  }
}

NodeOutput *Block::texture_output()
{
  return texture_output_;
}

NodeOutput *Block::block_output()
{
  return block_output_;
}

void Block::ConnectBlocks(Block *previous, Block *next)
{
  NodeParam::ConnectEdge(previous->block_output(), next->previous_input());
}

void Block::DisconnectBlocks(Block *previous, Block *next)
{
  NodeParam::DisconnectEdge(previous->block_output(), next->previous_input());
}

const rational &Block::media_in()
{
  return media_in_;
}

void Block::set_media_in(const rational &media_in)
{
  if (media_in_ != media_in) {
    media_in_ = media_in;

    // Signal that this clips contents have changed
    InvalidateCache(nullptr, in(), out());
  }
}

QList<NodeDependency> Block::RunDependencies(NodeOutput* param, const rational &time)
{
  // Base Blocks have no direct dependencies

  Q_UNUSED(param)
  Q_UNUSED(time)

  return QList<NodeDependency>();
}

rational Block::SequenceToMediaTime(const rational &sequence_time)
{
  // These constants are not considered "values" per se, so we don't modify them
  if (sequence_time == RATIONAL_MIN || sequence_time == RATIONAL_MAX) {
    return sequence_time;
  }

  return sequence_time - in() + media_in();
}

rational Block::MediaToSequenceTime(const rational &media_time)
{
  // These constants are not considered "values" per se, so we don't modify them
  if (media_time == RATIONAL_MIN || media_time == RATIONAL_MAX) {
    return media_time;
  }

  return media_time - media_in() + in();
}
