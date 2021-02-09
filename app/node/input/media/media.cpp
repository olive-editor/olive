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

const QString MediaInput::kFootageInput = QStringLiteral("footage_in");

MediaInput::MediaInput() :
  connected_footage_(nullptr)
{
  AddInput(kFootageInput, NodeValue::kFootage, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));
}

QVector<Node::CategoryID> MediaInput::Category() const
{
  return {kCategoryInput};
}

Stream *MediaInput::stream() const
{
  return Node::ValueToPtr<Stream>(GetStandardValue(kFootageInput));
}

void MediaInput::SetStream(Stream* s)
{
  SetStandardValue(kFootageInput, Node::PtrToValue(s));
}

void MediaInput::Retranslate()
{
  SetInputName(kFootageInput, tr("Media"));
}

NodeValueTable MediaInput::Value(const QString &output, NodeValueDatabase &value) const
{
  Q_UNUSED(output)

  NodeValueTable table = value.Merge();

  if (connected_footage_) {
    rational media_duration = Timecode::timestamp_to_time(connected_footage_->duration(),
                                                          connected_footage_->timebase());

    table.Push(NodeValue::kRational, QVariant::fromValue(media_duration), this, false, QStringLiteral("length"));
  }

  return table;
}

void MediaInput::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(element)

  if (input == kFootageInput) {
    Stream* new_footage = stream();

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
}

void MediaInput::FootageParametersChanged()
{
  InvalidateCache(TimeRange(0, RATIONAL_MAX), kFootageInput);
}

}
