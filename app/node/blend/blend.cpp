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

#include "blend.h"

BlendNode::BlendNode()
{
  base_input_ = new NodeInput("base_in");
  base_input_->add_data_input(NodeParam::kTexture);
  AddParameter(base_input_);

  blend_input_ = new NodeInput("blend_in");
  blend_input_->add_data_input(NodeParam::kTexture);
  AddParameter(blend_input_);

  texture_output_ = new NodeOutput("tex_out");
  texture_output_->set_data_type(NodeParam::kTexture);
  AddParameter(texture_output_);
}

QString BlendNode::Category()
{
  return tr("Blend");
}

NodeInput *BlendNode::base_input()
{
  return base_input_;
}

NodeInput *BlendNode::blend_input()
{
  return blend_input_;
}

NodeOutput *BlendNode::texture_output()
{
  return texture_output_;
}
