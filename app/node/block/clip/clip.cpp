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

namespace olive {

#define super Block

const QString ClipBlock::kBufferIn = QStringLiteral("buffer_in");

ClipBlock::ClipBlock(bool create_buffer_in)
{
  if (create_buffer_in) {
    AddInput(kBufferIn, NodeValue::kNone, InputFlags(kInputFlagNotKeyframable));
  }
}

Node *ClipBlock::copy() const
{
  return new ClipBlock();
}

QString ClipBlock::Name() const
{
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

void ClipBlock::InvalidateCache(const TimeRange& range, const QString& from, int element, qint64 job_time)
{
  Q_UNUSED(element)

  // If signal is from texture input, transform all times from media time to sequence time
  if (from == kBufferIn) {
    // Adjust range from media time to sequence time
    rational start = MediaToSequenceTime(range.in());
    rational end = MediaToSequenceTime(range.out());

    super::InvalidateCache(TimeRange(start, end), from, element, job_time);
  } else {
    // Otherwise, pass signal along normally
    super::InvalidateCache(range, from, element, job_time);
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

NodeValueTable ClipBlock::Value(const QString &output, NodeValueDatabase &value) const
{
  Q_UNUSED(output)

  // We discard most values here except for the buffer we received
  NodeValue data = value[kBufferIn].GetWithMeta(NodeValue::kBuffer);

  NodeValueTable table;
  if (data.type() != NodeValue::kNone) {
    table.Push(data);
  }
  return table;
}

void ClipBlock::Retranslate()
{
  super::Retranslate();

  if (HasInputWithID(kBufferIn)) {
    SetInputName(kBufferIn, tr("Buffer"));
  }
}

void ClipBlock::Hash(const QString &out, QCryptographicHash &hash, const rational &time, const VideoParams &video_params) const
{
  Q_UNUSED(out)

  if (IsInputConnected(kBufferIn)) {
    rational t = InputTimeAdjustment(kBufferIn, -1, TimeRange(time, time)).in();

    NodeOutput output = GetConnectedOutput(kBufferIn);
    output.node()->Hash(output.output(), hash, t, video_params);
  }
}

}
