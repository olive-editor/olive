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

#include "opacity.h"

OpacityNode::OpacityNode()
{
  opacity_input_ = new NodeInput("opacity_in");
  opacity_input_->set_data_type(NodeParam::kFloat);
  opacity_input_->set_value_at_time(0, 100);
  opacity_input_->set_minimum(0);
  opacity_input_->set_maximum(100);
  AddParameter(opacity_input_);

  texture_input_ = new NodeInput("tex_in");
  texture_input_->set_data_type(NodeParam::kTexture);
  AddParameter(texture_input_);
}

Node *OpacityNode::copy() const
{
  return new OpacityNode();
}

QString OpacityNode::Name() const
{
  return tr("Opacity");
}

QString OpacityNode::Category() const
{
  return tr("Color");
}

QString OpacityNode::Description() const
{
  return tr("Adjust an image's opacity.");
}

QString OpacityNode::id() const
{
  return "org.olivevideoeditor.Olive.opacity";
}

void OpacityNode::Retranslate()
{
  opacity_input_->set_name(tr("Opacity"));
}

QString OpacityNode::Code() const
{
  return "#version 110"
         "\n"
         "varying vec2 olive_tex_coord;\n"
         "\n"
         "uniform sampler2D tex_in;\n"
         "uniform float opacity_in;\n"
         "\n"
         "void main(void) {\n"
         "  gl_FragColor = texture2D(tex_in, olive_tex_coord) * (opacity_in * 0.01);\n"
         "}\n";
}

NodeInput *OpacityNode::texture_input() const
{
  return texture_input_;
}
