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
  connected_footage_(nullptr)
{
  footage_input_ = new NodeInput("footage_in");
  footage_input_->set_data_type(NodeInput::kFootage);
  footage_input_->SetConnectable(false);
  connect(footage_input_, SIGNAL(ValueChanged(const rational&, const rational&)), this, SLOT(FootageChanged()));
  AddInput(footage_input_);
}

StreamPtr MediaInput::footage()
{
  return footage_input_->get_value_at_time(0).value<StreamPtr>();
}

void MediaInput::SetFootage(StreamPtr f)
{
  footage_input_->set_value_at_time(0, QVariant::fromValue(f));
}

void MediaInput::Retranslate()
{
  footage_input_->set_name(tr("Footage"));
}

void MediaInput::FootageChanged()
{
  StreamPtr new_footage = footage_input_->get_value_at_time(0).value<StreamPtr>();

  if (new_footage == connected_footage_) {
    return;
  }

  if (connected_footage_ != nullptr) {
    disconnect(connected_footage_.get(), SIGNAL(ColorSpaceChanged()), this, SLOT(FootageColorSpaceChanged()));
  }

  connected_footage_ = new_footage;

  if (connected_footage_ != nullptr) {
    connect(connected_footage_.get(), SIGNAL(ColorSpaceChanged()), this, SLOT(FootageColorSpaceChanged()));
  }
}

void MediaInput::FootageColorSpaceChanged()
{
  InvalidateCache(0, RATIONAL_MAX, footage_input_);
}
