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

#include "transition/transition.h"

Block::Block() :
  previous_(nullptr),
  next_(nullptr)
{
  length_input_ = new NodeInput("length_in", NodeParam::kRational);
  length_input_->SetConnectable(false);
  length_input_->set_is_keyframable(false);
  AddInput(length_input_);
  connect(length_input_, SIGNAL(ValueChanged(const rational&, const rational&)), this, SLOT(LengthInputChanged()));

  media_in_input_ = new NodeInput("media_in_in", NodeParam::kRational);
  media_in_input_->SetConnectable(false);
  media_in_input_->set_is_keyframable(false);
  AddInput(media_in_input_);

  media_out_input_ = new NodeInput("media_out_in", NodeParam::kRational);
  media_out_input_->SetConnectable(false);
  media_out_input_->set_is_keyframable(false);
  AddInput(media_out_input_);

  // A block's length must be greater than 0
  set_length_and_media_out(1);
}

QString Block::Category() const
{
  return tr("Block");
}

const rational &Block::in() const
{
  return in_point_;
}

const rational &Block::out() const
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

rational Block::length() const
{
  return length_input_->get_value_at_time(0).value<rational>();
}

void Block::set_length(const rational &length)
{
  Q_ASSERT(length > 0);

  if (length == this->length()) {
    return;
  }

  length_input_->set_override_value(QVariant::fromValue(length));
}

void Block::set_length_and_media_out(const rational &length)
{
  Q_ASSERT(length > 0);

  if (length == this->length()) {
    return;
  }

  rational media_out_diff = length - this->length();

  // Try to maintain the same speed (which is determined by the media in to out points)
  if (media_length() != this->length()) {
    media_out_diff  = media_out_diff / this->length() * media_length();
  }

  set_media_out(media_out() + media_out_diff);

  set_length(length);
}

void Block::set_length_and_media_in(const rational &length)
{
  Q_ASSERT(length > 0);

  if (length == this->length()) {
    return;
  }

  rational media_in_diff = this->length() - length;

  // Try to maintain the same speed (which is determined by the media in to out points)
  if (media_length() != this->length()) {
    media_in_diff  = media_in_diff / this->length() * media_length();
  }

  // Calculate media_in adjustment
  set_media_in(media_in() + media_in_diff);

  // Set the length without setting media out
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

rational Block::media_in() const
{
  return media_in_input_->get_value_at_time(0).value<rational>();
}

void Block::set_media_in(const rational &media_in)
{
  media_in_input_->set_override_value(QVariant::fromValue(media_in));
}

rational Block::media_out() const
{
  return media_out_input_->get_value_at_time(0).value<rational>();
}

void Block::set_media_out(const rational &media_out)
{
  media_out_input_->set_override_value(QVariant::fromValue(media_out));
}

rational Block::media_length() const
{
  return media_out() - media_in();
}

double Block::speed() const
{
  return qAbs(media_length().toDouble() / length().toDouble());
}

bool Block::is_still() const
{
  return (media_in() == media_out());
}

bool Block::is_reversed() const
{
  return (media_out() < media_in());
}

const QString &Block::block_name() const
{
  return block_name_;
}

void Block::set_block_name(const QString &name)
{
  block_name_ = name;
}

rational Block::SequenceToMediaTime(const rational &sequence_time) const
{
  // These constants are not considered "values" per se, so we don't modify them
  if (sequence_time == RATIONAL_MIN || sequence_time == RATIONAL_MAX) {
    return sequence_time;
  }

  return ((sequence_time - in()) * media_length() / length()) + media_in();
}

rational Block::MediaToSequenceTime(const rational &media_time) const
{
  // These constants are not considered "values" per se, so we don't modify them
  if (media_time == RATIONAL_MIN || media_time == RATIONAL_MAX) {
    return media_time;
  }

  return (media_time - media_in()) * length() / media_length() + in();
}

void Block::CopyParameters(const Block *source, Block *dest)
{
  dest->set_block_name(source->block_name());
  dest->set_media_in(source->media_in());

  if (source->type() == kTransition && dest->type() == kTransition) {
    const TransitionBlock* src_t = static_cast<const TransitionBlock*>(source);
    TransitionBlock* dst_t = static_cast<TransitionBlock*>(dest);

    dst_t->set_in_and_out_offset(src_t->in_offset(), src_t->out_offset());
  } else {
    dest->set_length_and_media_out(source->length());
  }
}

void Block::LengthInputChanged()
{
  emit LengthChanged(length());
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

bool Block::IsBlock() const
{
  return true;
}

void Block::Retranslate()
{
  Node::Retranslate();

  length_input_->set_name(tr("Length"));
  media_in_input_->set_name(tr("Media In"));
}

