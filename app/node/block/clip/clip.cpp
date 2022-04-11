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
#include "node/output/viewer/viewer.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/rationalslider.h"

namespace olive {

#define super Block

const QString ClipBlock::kBufferIn = QStringLiteral("buffer_in");
const QString ClipBlock::kMediaInInput = QStringLiteral("media_in_in");
const QString ClipBlock::kSpeedInput = QStringLiteral("speed_in");
const QString ClipBlock::kReverseInput = QStringLiteral("reverse_in");
const QString ClipBlock::kMaintainAudioPitchInput = QStringLiteral("maintain_audio_pitch_in");

ClipBlock::ClipBlock() :
  in_transition_(nullptr),
  out_transition_(nullptr),
  connected_viewer_(nullptr)
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

  AddInput(kMaintainAudioPitchInput, NodeValue::kBoolean, false, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  PrependInput(kBufferIn, NodeValue::kNone, InputFlags(kInputFlagNotKeyframable));
  SetValueHintForInput(kBufferIn, ValueHint(NodeValue::kBuffer));
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
    rational proposed_media_in = SequenceToMediaTime(this->length() - length, true);
    set_media_in(proposed_media_in);
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
    rational proposed_media_in = SequenceToMediaTime(this->length() - length, false, true);

    waveform_.TrimIn(proposed_media_in - media_in());

    set_media_in(proposed_media_in);
  } else {
    // Trim waveform out point
    waveform_.TrimIn(this->length() - length);
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

rational ClipBlock::SequenceToMediaTime(const rational &sequence_time, bool ignore_reverse, bool ignore_speed) const
{
  // These constants are not considered "values" per se, so we don't modify them
  if (sequence_time == RATIONAL_MIN || sequence_time == RATIONAL_MAX) {
    return sequence_time;
  }

  rational media_time = sequence_time;

  if (!ignore_speed) {
    double speed_value = speed();
    if (qIsNull(speed_value)) {
      // Effectively holds the frame at the in point
      media_time = 0;
    } else if (!qFuzzyCompare(speed_value, 1.0)) {
      // Multiply time
      media_time = rational::fromDouble(media_time.toDouble() * speed_value);
    }
  }

  if (reverse() && !ignore_reverse) {
    media_time = length() - media_time;
  }

  media_time += media_in();

  return media_time;
}

rational ClipBlock::MediaToSequenceTime(const rational &media_time) const
{
  // These constants are not considered "values" per se, so we don't modify them
  if (media_time == RATIONAL_MIN || media_time == RATIONAL_MAX) {
    return media_time;
  }

  rational sequence_time = media_time - media_in();

  if (reverse()) {
    sequence_time = length() - sequence_time;
  }

  double speed_value = speed();
  if (qIsNull(speed_value)) {
    // I don't know what to return here yet...
    sequence_time = rational::NaN;
  } else if (!qFuzzyCompare(speed_value, 1.0)) {
    // Divide time
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
    TimeRange adj;
    double speed_value = speed();

    if (qIsNull(speed_value)) {
      // Handle 0 speed by invalidating the whole clip
      adj = TimeRange(RATIONAL_MIN, RATIONAL_MAX);
    } else {
      adj = TimeRange(MediaToSequenceTime(range.in()), MediaToSequenceTime(range.out()));
    }

    // Find connected viewer node
    auto viewers = FindInputNodesConnectedToInput<ViewerOutput>(NodeInput(this, kBufferIn));
    if (viewers.isEmpty()) {
      connected_viewer_ = nullptr;
    } else {
      connected_viewer_ = viewers.first();
    }

    super::InvalidateCache(adj, from, element, options);
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
