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
  previous_(nullptr),
  next_(nullptr)
{
  block_output_ = new NodeOutput("this_out");
  AddParameter(block_output_);

  buffer_output_ = new NodeOutput("buffer_out");
  AddParameter(buffer_output_);
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

void Block::set_in(const rational &in)
{
  in_point_ = in;
}

void Block::set_out(const rational &out)
{
  out_point_ = out;
}

const rational& Block::length()
{
  return length_;
}

void Block::set_length(const rational &length)
{
  Q_ASSERT(length > 0);

  if (length == length_) {
    return;
  }

  LockUserInput();

  length_ = length;

  UnlockUserInput();

  emit LengthChanged(length_);
}

void Block::set_length_and_media_in(const rational &length)
{
  // Calculate media_in adjustment
  set_media_in(media_in_ + (length_ - length));

  // Set the length
  set_length(length);
}

Block *Block::previous()
{
  return previous_;
}

Block *Block::next()
{
  return next_;
}

void Block::set_previous(Block *previous)
{
  previous_ = previous;
}

void Block::set_next(Block *next)
{
  next_ = next;
}

QVariant Block::Value(NodeOutput *output)
{
  if (output == block_output_) {
    // Simply set the output value to a pointer to this Block
    return PtrToValue(this);
  }

  return 0;
}

NodeOutput *Block::buffer_output()
{
  return buffer_output_;
}

NodeOutput *Block::block_output()
{
  return block_output_;
}

const rational &Block::media_in()
{
  return media_in_;
}

void Block::set_media_in(const rational &media_in)
{
  bool changed = false;

  LockUserInput();
  if (media_in_ != media_in) {
    media_in_ = media_in;
    changed = true;
  }
  UnlockUserInput();

  if (changed) {
    // Signal that this clips contents have changed
    SendInvalidateCache(in(), out());
  }
}

const QString &Block::block_name()
{
  return block_name_;
}

void Block::set_block_name(const QString &name)
{
  block_name_ = name;
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

void Block::CopyParameters(Block *source, Block *dest)
{
  dest->set_block_name(source->block_name());
  dest->set_length(source->length());
  dest->set_media_in(source->media_in());
}

void Block::Link(Block *a, Block *b)
{
  if (a == b || a == nullptr || b == nullptr) {
    return;
  }

  // Assume both clips are already linked since Link() and Unlink() should be the only entry points to this array
  if (a->linked_clips_.contains(b)) {
    return;
  }

  a->linked_clips_.append(b);
  b->linked_clips_.append(a);
}

void Block::Link(QList<Block *> blocks)
{
  foreach (Block* a, blocks) {
    foreach (Block* b, blocks) {
      Link(a, b);
    }
  }
}

void Block::Unlink(Block *a, Block *b)
{
  a->linked_clips_.removeOne(b);
  b->linked_clips_.removeOne(a);
}

bool Block::AreLinked(Block *a, Block *b)
{
  return a->linked_clips_.contains(b);
}

const QVector<Block*> &Block::linked_clips()
{
  return linked_clips_;
}

bool Block::HasLinks()
{
  return !linked_clips_.isEmpty();
}

bool Block::IsBlock()
{
  return true;
}

