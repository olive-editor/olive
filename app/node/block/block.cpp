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

Block::Block()
{
  previous_input_ = new NodeInput();
  previous_input_->add_data_input(NodeParam::kBlock);
  AddParameter(previous_input_);

  block_output_ = new NodeOutput();
  block_output_->set_data_type(NodeParam::kBlock);
  AddParameter(block_output_);

  next_input_ = new NodeInput();
  next_input_->add_data_input(NodeParam::kBlock);
  AddParameter(next_input_);

  texture_output_ = new NodeOutput();
  texture_output_->set_data_type(NodeParam::kTexture);
  AddParameter(texture_output_);
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

void Block::set_length(const rational &length)
{
  Q_UNUSED(length)
}

Block *Block::previous()
{
  return ValueToPtr<Block>(previous_input_->get_value(0));
}

Block *Block::next()
{
  return ValueToPtr<Block>(next_input_->get_value(0));
}

NodeInput *Block::previous_input()
{
  return previous_input_;
}

NodeInput *Block::next_input()
{
  return next_input_;
}

void Block::Process(const rational &time)
{
  Q_UNUSED(time)

  // Simply set both output values as a pointer to this object
  block_output_->set_value(PtrToValue(this));
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
}

void Block::RefreshSurrounds()
{
  Block* acting_node = previous();

  while (acting_node != nullptr) {
    acting_node->Refresh();
    acting_node = acting_node->previous();
  }

  acting_node = next();

  while (acting_node != nullptr) {
    acting_node->Refresh();
    acting_node = acting_node->next();
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
