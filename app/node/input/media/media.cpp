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

#include "project/item/footage/stream.h"

namespace olive {

MediaInput::MediaInput() :
  connected_footage_(nullptr)
{
  footage_input_ = new NodeInput("footage_in", NodeInput::kFootage);
  footage_input_->set_connectable(false);
  footage_input_->set_is_keyframable(false);
  connect(footage_input_, &NodeInput::ValueChanged, this, &MediaInput::FootageChanged);
  AddInput(footage_input_);
}

QVector<Node::CategoryID> MediaInput::Category() const
{
  return {kCategoryInput};
}

StreamPtr MediaInput::stream()
{
  return footage_input_->get_standard_value().value<StreamPtr>();
}

void MediaInput::SetStream(StreamPtr s)
{
  footage_input_->set_standard_value(QVariant::fromValue(s));
}

bool MediaInput::IsMedia() const
{
  return true;
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

    table.Push(NodeInput::kRational, QVariant::fromValue(media_duration), this, "length");
  }

  return table;
}

void MediaInput::FootageChanged()
{
  StreamPtr new_footage = footage_input_->get_standard_value().value<StreamPtr>();
  
  if (new_footage == connected_footage_) {
    return;
  }

  if (connected_footage_) {
    disconnect(connected_footage_.get(), &Stream::ParametersChanged, this, &MediaInput::FootageParametersChanged);
  }

  connected_footage_ = new_footage;

  if (connected_footage_) {
    connect(connected_footage_.get(), &Stream::ParametersChanged, this, &MediaInput::FootageParametersChanged);
    
    // If no user-defined label is set
    if (!HasUserLabel()){
      switch (connected_footage_->type()) {
        case olive::Stream::Type::kVideo:
          SetLabel(tr("Video Input"), false);
          break;
        case olive::Stream::Type::kAudio:
          SetLabel(tr("Audio Input"), false);
          break;
        default:
          break;
      }
    }
  }
}

void MediaInput::FootageParametersChanged()
{
  InvalidateCache(TimeRange(0, RATIONAL_MAX), footage_input_, footage_input_);
}

}
