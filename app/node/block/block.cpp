/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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
#include "widget/slider/floatslider.h"

namespace olive {

const QString Block::kLengthInput = QStringLiteral("length_in");
const QString Block::kMediaInInput = QStringLiteral("media_in_in");
const QString Block::kEnabledInput = QStringLiteral("enabled_in");
const QString Block::kSpeedInput = QStringLiteral("speed_in");

Block::Block() :
  previous_(nullptr),
  next_(nullptr),
  track_(nullptr),
  index_(-1),
  in_transition_(nullptr),
  out_transition_(nullptr)
{
  AddInput(kLengthInput, NodeValue::kRational, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));
  IgnoreInvalidationsFrom(kLengthInput);
  IgnoreHashingFrom(kLengthInput);

  AddInput(kMediaInInput, NodeValue::kRational, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));
  IgnoreHashingFrom(kMediaInInput);

  AddInput(kEnabledInput, NodeValue::kBoolean, true, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  AddInput(kSpeedInput, NodeValue::kFloat, 1.0);
  SetInputProperty(kSpeedInput, QStringLiteral("view"), FloatSlider::kPercentage);
  IgnoreHashingFrom(kSpeedInput);

  // A block's length must be greater than 0
  set_length_and_media_out(1);
}

QVector<Node::CategoryID> Block::Category() const
{
  return {kCategoryTimeline};
}

rational Block::length() const
{
  return GetStandardValue(kLengthInput).value<rational>();
}

void Block::set_length_and_media_out(const rational &length)
{
  Q_ASSERT(length > 0);

  if (length == this->length()) {
    return;
  }

  set_length_internal(length);
}

void Block::set_length_and_media_in(const rational &length)
{
  Q_ASSERT(length > 0);

  if (length == this->length()) {
    return;
  }

  // Calculate media_in adjustment
  set_media_in(SequenceToMediaTime(this->length() - length));

  // Set the length without setting media out
  set_length_internal(length);
}

rational Block::media_in() const
{
  return GetStandardValue(kMediaInInput).value<rational>();
}

void Block::set_media_in(const rational &media_in)
{
  SetStandardValue(kMediaInInput, QVariant::fromValue(media_in));
}

bool Block::is_enabled() const
{
  return GetStandardValue(kEnabledInput).toBool();
}

void Block::set_enabled(bool e)
{
  SetStandardValue(kEnabledInput, e);

  emit EnabledChanged();
}

rational Block::SequenceToMediaTime(const rational &sequence_time) const
{
  // These constants are not considered "values" per se, so we don't modify them
  if (sequence_time == RATIONAL_MIN || sequence_time == RATIONAL_MAX) {
    return sequence_time;
  }

  rational local_time = sequence_time;

  // FIXME: Doesn't handle reversing
  if (IsInputStatic(kSpeedInput)) {
    double speed_value = GetStandardValue(kSpeedInput).toDouble();

    if (qIsNull(speed_value)) {
      // Effectively holds the frame at the in point
      local_time = 0;
    } else if (!qFuzzyCompare(speed_value, 1.0)) {
      // Multiply time
      local_time = rational::fromDouble(local_time.toDouble() * speed_value);
    }
  } else {
    // FIXME: We'll need to calculate the speed hoo boy
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
  if (IsInputKeyframing(kSpeedInput) || IsInputConnected(kSpeedInput)) {
    // FIXME: We'll need to calculate the speed hoo boy
  } else {
    double speed_value = GetStandardValue(kSpeedInput).toDouble();

    if (qIsNull(speed_value)) {
      // Effectively holds the frame at the in point, also prevents divide by zero
      sequence_time = 0;
    } else if (!qFuzzyCompare(speed_value, 1.0)) {
      // Multiply time
      sequence_time = rational::fromDouble(sequence_time.toDouble() / speed_value);
    }
  }

  return sequence_time;
}

void Block::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(element)

  if (input == kLengthInput) {
    emit LengthChanged();
  } else if (input == kEnabledInput) {
    emit EnabledChanged();
  }
}

void Block::LinkChangeEvent()
{
  block_links_.clear();

  foreach (Node* n, links()) {
    Block* b = dynamic_cast<Block*>(n);

    if (b) {
      block_links_.append(b);
    }
  }
}

void Block::set_length_internal(const rational &length)
{
  SetStandardValue(kLengthInput, QVariant::fromValue(length));
}

void Block::Retranslate()
{
  Node::Retranslate();

  SetInputName(kLengthInput, tr("Length"));
  SetInputName(kMediaInInput, tr("Media In"));
  SetInputName(kEnabledInput, tr("Enabled"));
  SetInputName(kSpeedInput, tr("Speed"));
}

void Block::Hash(const QString &, QCryptographicHash &, const rational &) const
{
  // A block does nothing by default, so we hash nothing
}

}
