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

#include "media.h"

#include "common/timecodefunctions.h"
#include "common/tohex.h"

namespace olive {

MediaInput::MediaInput() :
  connected_footage_(nullptr)
{
  footage_input_ = new NodeInput(this, QStringLiteral("footage_in"), NodeValue::kFootage);
  footage_input_->SetConnectable(false);
  footage_input_->SetKeyframable(false);
  connect(footage_input_, &NodeInput::ValueChanged, this, &MediaInput::FootageChanged);
}

QVector<Node::CategoryID> MediaInput::Category() const
{
  return {kCategoryInput};
}

Stream *MediaInput::stream() const
{
  return Node::ValueToPtr<Stream>(footage_input_->GetStandardValue());
}

void MediaInput::SetStream(Stream* s)
{
  footage_input_->SetStandardValue(Node::PtrToValue(s));
}

void MediaInput::Retranslate()
{
  footage_input_->set_name(tr("Media"));
}

NodeValueTable MediaInput::Value(NodeValueDatabase &value) const
{
  NodeValueTable table = value.Merge();

  if (connected_footage_) {
    rational media_duration = Timecode::timestamp_to_time(connected_footage_->duration(),
                                                          connected_footage_->timebase());

    table.Push(NodeValue::kRational, QVariant::fromValue(media_duration), this, false, QStringLiteral("length"));
  }

  return table;
}

void MediaInput::FootageChanged()
{
  Stream* new_footage = footage_input_->GetStandardValue().value<Stream*>();

  if (new_footage == connected_footage_) {
    return;
  }

  if (connected_footage_) {
    disconnect(connected_footage_, &Stream::ParametersChanged, this, &MediaInput::FootageParametersChanged);
  }

  connected_footage_ = new_footage;

  if (connected_footage_) {
    connect(connected_footage_, &Stream::ParametersChanged, this, &MediaInput::FootageParametersChanged);
  }
}

void MediaInput::FootageParametersChanged()
{
  InvalidateCache(TimeRange(0, RATIONAL_MAX), InputConnection(footage_input_));
}

}
