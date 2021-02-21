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

#include "clip.h"

namespace olive {

const QString ClipBlock::kBufferIn = QStringLiteral("buffer_in");

ClipBlock::ClipBlock()
{
  AddInput(kBufferIn, NodeValue::kNone, InputFlags(kInputFlagNotKeyframable));
}

Node *ClipBlock::copy() const
{
  return new ClipBlock();
}

Block::Type ClipBlock::type() const
{
  return kClip;
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

    Block::InvalidateCache(TimeRange(start, end), from, element, job_time);
  } else {
    // Otherwise, pass signal along normally
    Block::InvalidateCache(range, from, element, job_time);
  }
}

TimeRange ClipBlock::InputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const
{
  Q_UNUSED(element)

  if (input == kBufferIn) {
    return TimeRange(SequenceToMediaTime(input_time.in()), SequenceToMediaTime(input_time.out()));
  }

  return Block::InputTimeAdjustment(input, element, input_time);
}

TimeRange ClipBlock::OutputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const
{
  Q_UNUSED(element)

  if (input == kBufferIn) {
    return TimeRange(MediaToSequenceTime(input_time.in()), MediaToSequenceTime(input_time.out()));
  }

  return Block::OutputTimeAdjustment(input, element, input_time);
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
  Block::Retranslate();

  SetInputName(kBufferIn, tr("Buffer"));
}

void ClipBlock::Hash(const QString &output, QCryptographicHash &hash, const rational &time) const
{
  if (IsInputConnected(kBufferIn)) {
    rational t = InputTimeAdjustment(kBufferIn, -1, TimeRange(time, time)).in();

    GetConnectedNode(kBufferIn)->Hash(output, hash, t);
  }
}

}
