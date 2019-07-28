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
  texture_input_ = new NodeInput();
  texture_input_->add_data_input(NodeInput::kTexture);
  AddParameter(texture_input_);
}

NodeInput *ClipBlock::texture_input()
{
  return texture_input_;
}

void ClipBlock::Process(const rational &time)
{
  // Run default node processing
  Block::Process(time);

  // We convert the time given (timeline time) to media time
  rational media_time = time - in() + media_in_;

  // Retrieve texture
  texture_output()->set_value(texture_input_->get_value(media_time));
}
