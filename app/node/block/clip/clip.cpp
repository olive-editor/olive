/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "clip.h"

#include "node/output/track/track.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/rationalslider.h"

namespace olive {

#define super Block

const QString ClipBlock::kBufferIn = QStringLiteral("buffer_in");
const QString ClipBlock::kMediaInInput = QStringLiteral("media_in_in");
const QString ClipBlock::kSpeedInput = QStringLiteral("speed_in");
const QString ClipBlock::kReverseInput = QStringLiteral("reverse_in");

ClipBlock::ClipBlock() :
  in_transition_(nullptr),
  out_transition_(nullptr)
{
  AddInput(kMediaInInput, NodeValue::kRational, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));
  SetInputProperty(kMediaInInput, QStringLiteral("view"), RationalSlider::kTime);
  SetInputProperty(kMediaInInput, QStringLiteral("viewlock"), true);
  IgnoreHashingFrom(kMediaInInput);

  AddInput(kSpeedInput, NodeValue::kFloat, 1.0, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));
  SetInputProperty(kSpeedInput, QStringLiteral("view"), FloatSlider::kPercentage);
  SetInputProperty(kSpeedInput, QStringLiteral("min"), 0.0);
  IgnoreHashingFrom(kSpeedInput);

  AddInput(kReverseInput, NodeValue::kBoolean, false, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));
  IgnoreHashingFrom(kReverseInput);

  PrependInput(kBufferIn, NodeValue::kNone, InputFlags(kInputFlagNotKeyframable));
  SetValueHintForInput(kBufferIn, {NodeValue::kBuffer, -1, QString()});
}

Node *ClipBlock::copy() const
{
  return new ClipBlock();
}

QString ClipBlock::Name() const
{
  if (track()) {
    if (track()->type() == Track::kVideo) {
      return tr("Video Clip");
    } else if (track()->type() == Track::kAudio) {
      return tr("Audio Clip");
    }
  }

  return tr("Clip");
}

QString ClipBlock::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.clip");
}

QString ClipBlock::Description() const
{
  return tr("A time-based node that represents a media source.");
}

void ClipBlock::set_length_and_media_out(const rational &length)
{
  if (length == this->length()) {
    return;
  }

  if (reverse()) {
    // Calculate media_in adjustment
    set_media_in(SequenceToMediaTime(length - this->length(), true));
  }

  super::set_length_and_media_out(length);
}

void ClipBlock::set_length_and_media_in(const rational &length)
{
  if (length == this->length()) {
    return;
  }

  if (!reverse()) {
    // Calculate media_in adjustment
    set_media_in(SequenceToMediaTime(this->length() - length));
  }

  super::set_length_and_media_in(length);
}

rational ClipBlock::media_in() const
{
  return GetStandardValue(kMediaInInput).value<rational>();
}

void ClipBlock::set_media_in(const rational &media_in)
{
  SetStandardValue(kMediaInInput, QVariant::fromValue(media_in));
}

rational ClipBlock::SequenceToMediaTime(const rational &sequence_time, bool ignore_reverse) const
{
  // These constants are not considered "values" per se, so we don't modify them
  if (sequence_time == RATIONAL_MIN || sequence_time == RATIONAL_MAX) {
    return sequence_time;
  }

  rational local_time = sequence_time;

  double speed_value = speed();

  if (qIsNull(speed_value)) {
    // Effectively holds the frame at the in point
    local_time = 0;
  } else if (!qFuzzyCompare(speed_value, 1.0)) {
    // Multiply time
    local_time = rational::fromDouble(local_time.toDouble() * speed_value);
  }

  rational media_time = local_time + media_in();

  if (reverse() && !ignore_reverse) {
    media_time = length() - media_time;
  }

  return media_time;
}

rational ClipBlock::MediaToSequenceTime(const rational &media_time) const
{
  // These constants are not considered "values" per se, so we don't modify them
  if (media_time == RATIONAL_MIN || media_time == RATIONAL_MAX) {
    return media_time;
  }

  rational sequence_time = media_time;

  if (reverse()) {
    sequence_time = length() - sequence_time;
  }

  sequence_time -= media_in();

  double speed_value = speed();

  if (qIsNull(speed_value)) {
    // Effectively holds the frame at the in point, also prevents divide by zero
    sequence_time = 0;
  } else if (!qFuzzyCompare(speed_value, 1.0)) {
    // Multiply time
    sequence_time = rational::fromDouble(sequence_time.toDouble() / speed_value);
  }

  return sequence_time;
}

void ClipBlock::InvalidateCache(const TimeRange& range, const QString& from, int element, InvalidateCacheOptions options)
{
  Q_UNUSED(element)

  // If signal is from texture input, transform all times from media time to sequence time
  if (from == kBufferIn) {
    // Adjust range from media time to sequence time
    rational start = MediaToSequenceTime(range.in());
    rational end = MediaToSequenceTime(range.out());

    super::InvalidateCache(TimeRange(start, end), from, element, options);
  } else {
    // Otherwise, pass signal along normally
    super::InvalidateCache(range, from, element, options);
  }
}

void ClipBlock::LinkChangeEvent()
{
  block_links_.clear();

  foreach (Node* n, links()) {
    ClipBlock* b = dynamic_cast<ClipBlock*>(n);

    if (b) {
      block_links_.append(b);
    }
  }
}

void ClipBlock::InputValueChangedEvent(const QString &input, int element)
{
  super::InputValueChangedEvent(input, element);

  if (input == kMediaInInput) {
    // Shift waveform in the inverse that the media in moved
    rational diff = media_in() - last_media_in_;
    waveform_.TrimIn(diff);

    last_media_in_ = media_in();
  }
}

TimeRange ClipBlock::InputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const
{
  Q_UNUSED(element)

  if (input == kBufferIn) {
    return TimeRange(SequenceToMediaTime(input_time.in()), SequenceToMediaTime(input_time.out()));
  }

  return super::InputTimeAdjustment(input, element, input_time);
}

TimeRange ClipBlock::OutputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const
{
  Q_UNUSED(element)

  if (input == kBufferIn) {
    return TimeRange(MediaToSequenceTime(input_time.in()), MediaToSequenceTime(input_time.out()));
  }

  return super::OutputTimeAdjustment(input, element, input_time);
}

void ClipBlock::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  Q_UNUSED(globals)

  // We discard most values here except for the buffer we received
  NodeValue data = value[kBufferIn];

  table->Clear();
  if (data.type() != NodeValue::kNone) {
    table->Push(data);
  }
}

void ClipBlock::Retranslate()
{
  super::Retranslate();

  SetInputName(kBufferIn, tr("Buffer"));
  SetInputName(kMediaInInput, tr("Media In"));
  SetInputName(kSpeedInput, tr("Speed"));
  SetInputName(kReverseInput, tr("Reverse"));
}

void ClipBlock::Hash(QCryptographicHash &hash, const NodeGlobals &globals, const VideoParams &video_params) const
{
  HashPassthrough(kBufferIn, hash, globals, video_params);
}

}
