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

#include "clip.h"

ClipBlock::ClipBlock()
{
  texture_input_ = new NodeInput("buffer_in", NodeInput::kBuffer);
  texture_input_->set_is_keyframable(false);
  AddInput(texture_input_);
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
  return "org.olivevideoeditor.Olive.clip";
}

QString ClipBlock::Description() const
{
  return tr("A time-based node that represents a media source.");
}

NodeInput *ClipBlock::texture_input() const
{
  return texture_input_;
}

void ClipBlock::InvalidateCache(const rational &start_range, const rational &end_range, NodeInput *from)
{
  // If signal is from texture input, transform all times from media time to sequence time
  if (from == texture_input_) {
    rational start = MediaToSequenceTime(start_range);
    rational end = MediaToSequenceTime(end_range);

    // Ensure range actually covers this clip's area
    if (!(end < in() || start > out())) {

      // Limit cache invalidation to clip lengths
      start = qMax(start, in());
      end = qMin(end, out());

      Node::InvalidateCache(start, end, from);

    }
  } else {
    // Otherwise, pass signal along normally
    Node::InvalidateCache(start_range, end_range, from);
  }
}

TimeRange ClipBlock::InputTimeAdjustment(NodeInput *input, const TimeRange &input_time) const
{
  if (input == texture_input_) {
    return TimeRange(SequenceToMediaTime(input_time.in()), SequenceToMediaTime(input_time.out()));
  }

  return Block::InputTimeAdjustment(input, input_time);
}

TimeRange ClipBlock::OutputTimeAdjustment(NodeInput *input, const TimeRange &input_time) const
{
  if (input == texture_input_) {
    return TimeRange(MediaToSequenceTime(input_time.in()), MediaToSequenceTime(input_time.out()));
  }

  return Block::InputTimeAdjustment(input, input_time);
}

NodeValueTable ClipBlock::Value(const NodeValueDatabase &value) const
{
  // We discard most values here except for the buffer we received
  NodeValueTable table;
  NodeValue data = value[texture_input()].GetWithMeta(NodeParam::kBuffer);

  if (data.type() != NodeParam::kNone) {
    table.Push(data);
  }

  return table;
}

void ClipBlock::Retranslate()
{
  Block::Retranslate();

  texture_input_->set_name(tr("Buffer"));
}
