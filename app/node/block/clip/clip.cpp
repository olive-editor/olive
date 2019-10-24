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
  texture_input_ = new NodeInput("tex_in");
  texture_input_->set_data_type(NodeInput::kTexture);
  AddParameter(texture_input_);
}

Block *ClipBlock::copy()
{
  ClipBlock* c = new ClipBlock();

  CopyParameters(this, c);

  return c;
}

Block::Type ClipBlock::type()
{
  return kClip;
}

QString ClipBlock::Name()
{
  return tr("Clip");
}

QString ClipBlock::id()
{
  return "org.olivevideoeditor.Olive.clip";
}

QString ClipBlock::Description()
{
  return tr("A time-based node that represents a media source.");
}

NodeInput *ClipBlock::texture_input()
{
  return texture_input_;
}

QVariant ClipBlock::Value(NodeOutput* param, const rational& v_in, const rational &v_out)
{
  if (param == buffer_output()) {
    // If the time retrieved is within this block, get texture information
    if (texture_input()->IsConnected() && v_in >= in() && v_out < out()) {
      // Retrieve texture
      // We convert the time given (timeline time) to media time
      return texture_input_->get_value(SequenceToMediaTime(v_in), SequenceToMediaTime(v_out));
    }
    return 0;
  }

  return Block::Value(param, v_in, v_out);
}

void ClipBlock::InvalidateCache(const rational &start_range, const rational &end_range, NodeInput *from)
{
  // If signal is from texture input, transform all times from media time to sequence time
  if (from == texture_input_) {
    rational start = MediaToSequenceTime(start_range);
    rational end = MediaToSequenceTime(end_range);

    // Limit cache invalidation to clip lengths
    start = qMax(start, in());
    end = qMin(end, out());

    Node::InvalidateCache(start, end, from);
  } else {
    // Otherwise, pass signal along normally
    Node::InvalidateCache(start_range, end_range, from);
  }
}

QList<NodeDependency> ClipBlock::RunDependencies(NodeOutput *output, const rational &time)
{
  QList<NodeDependency> deps;

  if (output == buffer_output() && texture_input_->IsConnected()) {
    deps.append(NodeDependency(texture_input_->get_connected_output(), SequenceToMediaTime(time), SequenceToMediaTime(time)));
  }

  return deps;
}
