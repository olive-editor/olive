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

MediaInput::MediaInput()
{
  footage_input_ = new NodeInput("footage_in");
  footage_input_->set_data_type(NodeInput::kFootage);
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
