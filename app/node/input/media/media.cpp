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

#include "media.h"

MediaInput::MediaInput() :
  decoder_(nullptr),
  frame_(nullptr)
{
  footage_input_ = new NodeInput("footage_in");
  footage_input_->add_data_input(NodeInput::kFootage);
  AddParameter(footage_input_);
}

void MediaInput::Release()
{
  frame_ = nullptr;
  decoder_ = nullptr;
}

StreamPtr MediaInput::footage()
{
  return footage_input_->get_value(0).value<StreamPtr>();
}

void MediaInput::SetFootage(StreamPtr f)
{
  footage_input_->set_value(QVariant::fromValue(f));
}

bool MediaInput::SetupDecoder()
{
  if (decoder_ != nullptr) {
    return true;
  }

  // Get currently selected Footage
  StreamPtr stream = footage();

  // If no footage is selected, return nothing
  if (stream == nullptr) {
    return false;
  }

  // Otherwise try to get frame of footage from decoder

  // Determine which decoder to use
  if (decoder_ == nullptr
      && (decoder_ = Decoder::CreateFromID(stream->footage()->decoder())) == nullptr) {
    return false;
  }

  if (decoder_->stream() == nullptr) {
    decoder_->set_stream(stream);
  }

  return true;
}
