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
  AddParameter(previous_input_);

  previous_output_ = new NodeOutput();
  AddParameter(previous_output_);

  next_input_ = new NodeInput();
  AddParameter(next_input_);

  next_output_ = new NodeOutput();
  AddParameter(next_output_);
}

QString Block::Category()
{
  return tr("Block");
}

rational Block::in()
{
  rational in_point = 0;

  Block* block = previous();

  while (block != nullptr) {
    in_point += block->length();

    block = previous();
  }

  return in_point;
}

rational Block::out()
{
  return in() + length();
}

Block *Block::previous()
{
  return ValueToPtr<Block>(previous_input_->get_value(0));
}

Block *Block::next()
{
  return ValueToPtr<Block>(next_input_->get_value(0));
}

void Block::Process(const rational &time)
{
  Q_UNUSED(time)

  // Simply set both output values as a pointer to this object
  previous_output_->set_value(PtrToValue(this));
  next_output_->set_value(PtrToValue(this));
}
