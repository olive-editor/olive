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

#include "node/output/track/track.h"
#include "transition/transition.h"

OLIVE_NAMESPACE_ENTER

Block::Block() :
  previous_(nullptr),
  next_(nullptr)
{
  length_input_ = new NodeInput("length_in", NodeParam::kRational);
  length_input_->set_connectable(false);
  length_input_->set_is_keyframable(false);
  AddInput(length_input_);
  disconnect(length_input_, &NodeInput::ValueChanged, this, &Block::InputChanged);
  connect(length_input_, &NodeInput::ValueChanged, this, &Block::LengthInputChanged);

  media_in_input_ = new NodeInput("media_in_in", NodeParam::kRational);
  media_in_input_->set_connectable(false);
  media_in_input_->set_is_keyframable(false);
  AddInput(media_in_input_);

  enabled_input_ = new NodeInput("enabled_in", NodeParam::kBoolean);
  enabled_input_->set_connectable(false);
  enabled_input_->set_is_keyframable(false);
  enabled_input_->set_standard_value(true);
  AddInput(enabled_input_);

  speed_input_ = new NodeInput("speed_in", NodeParam::kFloat);
  speed_input_->set_standard_value(1.0);
  AddInput(speed_input_);

  // A block's length must be greater than 0
  set_length_and_media_out(1);
}

QList<Node::CategoryID> Block::Category() const
{
  return {kCategoryTimeline};
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
  return length_input_->get_standard_value().value<rational>();
}

void Block::set_length_and_media_out(const rational &length)
{
  Q_ASSERT(length > 0);

  if (length == this->length()) {
    return;
  }

  rational old_length = this->length();

  set_length_internal(length);

  LengthChangedEvent(old_length, length, Timeline::kTrimOut);
}

void Block::set_length_and_media_in(const rational &length)
{
  Q_ASSERT(length > 0);

  if (length == this->length()) {
    return;
  }

  // Calculate media_in adjustment
  set_media_in(SequenceToMediaTime(in() + (this->length() - length)));

  rational old_length = this->length();

  // Set the length without setting media out
  set_length_internal(length);

  LengthChangedEvent(old_length, length, Timeline::kTrimIn);
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
  return media_in_input_->get_standard_value().value<rational>();
}

void Block::set_media_in(const rational &media_in)
{
  media_in_input_->set_standard_value(QVariant::fromValue(media_in));
}

bool Block::is_enabled() const
{
  return enabled_input_->get_standard_value().toBool();
}

void Block::set_enabled(bool e)
{
  enabled_input_->set_standard_value(e);

  emit EnabledChanged();
}

rational Block::SequenceToMediaTime(const rational &sequence_time) const
{
  // These constants are not considered "values" per se, so we don't modify them
  if (sequence_time == RATIONAL_MIN || sequence_time == RATIONAL_MAX) {
    return sequence_time;
  }

  rational local_time = sequence_time - in();

  // FIXME: Doesn't handle reversing
  if (speed_input_->is_keyframing() || speed_input_->is_connected()) {
    // FIXME: We'll need to calculate the speed hoo boy
  } else {
    double speed_value = speed_input_->get_standard_value().toDouble();

    if (qIsNull(speed_value)) {
      // Effectively holds the frame at the in point
      local_time = 0;
    } else if (!qFuzzyCompare(speed_value, 1.0)) {
      // Multiply time
      local_time = rational::fromDouble(local_time.toDouble() * speed_value);
    }
  }

  return local_time + media_in();
}

rational Block::MediaToSequenceTime(const rational &media_time) const
{
  // These constants are not considered "values" per se, so we don't modify them
  if (media_time == RATIONAL_MIN || media_time == RATIONAL_MAX) {
    return media_time;
  }

  rational sequence_time = media_time - media_in();

  // FIXME: Doesn't handle reversing
  if (speed_input_->is_keyframing() || speed_input_->is_connected()) {
    // FIXME: We'll need to calculate the speed hoo boy
  } else {
    double speed_value = speed_input_->get_standard_value().toDouble();

    if (qIsNull(speed_value)) {
      // Effectively holds the frame at the in point, also prevents divide by zero
      sequence_time = 0;
    } else if (!qFuzzyCompare(speed_value, 1.0)) {
      // Multiply time
      sequence_time = rational::fromDouble(sequence_time.toDouble() / speed_value);
    }
  }

  return sequence_time + in();
}

void Block::LoadInternal(QXmlStreamReader *reader, XMLNodeData &xml_node_data)
{
  if (reader->name() == QStringLiteral("link")) {
    xml_node_data.block_links.append({this, reader->readElementText().toULongLong()});
  } else {
    Node::LoadInternal(reader, xml_node_data);
  }
}

void Block::SaveInternal(QXmlStreamWriter *writer) const
{
  foreach (Block* link, linked_clips_) {
    writer->writeTextElement(QStringLiteral("link"), QString::number(reinterpret_cast<quintptr>(link)));
  }
}

QList<NodeInput *> Block::GetInputsToHash() const
{
  QList<NodeInput*> inputs = Node::GetInputsToHash();

  // Ignore these inputs
  inputs.removeOne(media_in_input_);
  inputs.removeOne(speed_input_);
  inputs.removeOne(length_input_);

  return inputs;
}

void Block::LengthChangedEvent(const rational &, const rational &, const Timeline::MovementMode &)
{
}

void Block::set_length_internal(const rational &length)
{
  length_input_->set_standard_value(QVariant::fromValue(length));
}

void Block::LengthInputChanged()
{
  emit LengthChanged(length());
}

bool Block::Link(Block *a, Block *b)
{
  if (a == b || !a || !b) {
    return false;
  }

  // Prevent duplicate link entries (assume that we only need to check one clip since this should be the only function
  // that adds to the linked array)
  if (Block::AreLinked(a, b)) {
    return false;
  }

  a->linked_clips_.append(b);
  b->linked_clips_.append(a);

  emit a->LinksChanged();
  emit b->LinksChanged();

  return true;
}

void Block::Link(const QList<Block*>& blocks)
{
  foreach (Block* a, blocks) {
    foreach (Block* b, blocks) {
      Link(a, b);
    }
  }
}

bool Block::Unlink(Block *a, Block *b)
{
  if (a == b || !a || !b) {
    return false;
  }

  if (!Block::AreLinked(a, b)) {
    return false;
  }

  a->linked_clips_.removeOne(b);
  b->linked_clips_.removeOne(a);

  emit a->LinksChanged();
  emit b->LinksChanged();

  return true;
}

void Block::Unlink(const QList<Block *> &blocks)
{
  foreach (Block* a, blocks) {
    foreach (Block* b, blocks) {
      Unlink(a, b);
    }
  }
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
  enabled_input_->set_name(tr("Enabled"));
  speed_input_->set_name(tr("Speed"));
}

NodeInput *Block::length_input() const
{
  return length_input_;
}

NodeInput *Block::media_in_input() const
{
  return media_in_input_;
}

NodeInput *Block::speed_input() const
{
  return speed_input_;
}

void Block::Hash(QCryptographicHash &, const rational &) const
{
  // A block does nothing by default, so we hash nothing
}

OLIVE_NAMESPACE_EXIT
