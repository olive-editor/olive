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

#include "blur.h"

OLIVE_NAMESPACE_ENTER

BlurFilterNode::BlurFilterNode()
{
  texture_input_ = new NodeInput("tex_in", NodeParam::kTexture);
  AddInput(texture_input_);

  method_input_ = new NodeInput("method_in", NodeParam::kCombo, 0);
  AddInput(method_input_);

  radius_input_ = new NodeInput("radius_in", NodeParam::kFloat, 10.0f);
  radius_input_->set_property(QStringLiteral("min"), 0.0f);
  AddInput(radius_input_);

  horiz_input_ = new NodeInput("horiz_in", NodeParam::kBoolean, true);
  AddInput(horiz_input_);

  vert_input_ = new NodeInput("vert_in", NodeParam::kBoolean, true);
  AddInput(vert_input_);

  repeat_edge_pixels_input_ = new NodeInput("repeat_edge_pixels_in", NodeParam::kBoolean, false);
  AddInput(repeat_edge_pixels_input_);
}

Node *BlurFilterNode::copy() const
{
  return new BlurFilterNode();
}

QString BlurFilterNode::Name() const
{
  return tr("Blur");
}

QString BlurFilterNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.blur");
}

QList<Node::CategoryID> BlurFilterNode::Category() const
{
  return {kCategoryFilter};
}

QString BlurFilterNode::Description() const
{
  return tr("Blurs an image.");
}

void BlurFilterNode::Retranslate()
{
  texture_input_->set_name(tr("Input"));
  method_input_->set_name(tr("Method"));
  method_input_->set_combobox_strings({ tr("Box"), tr("Gaussian") });
  radius_input_->set_name(tr("Radius"));
  horiz_input_->set_name(tr("Horizontal"));
  vert_input_->set_name(tr("Vertical"));
  repeat_edge_pixels_input_->set_name(tr("Repeat Edge Pixels"));
}

Node::Capabilities BlurFilterNode::GetCapabilities(const NodeValueDatabase &) const
{
  return kShader;
}

QString BlurFilterNode::ShaderFragmentCode(const NodeValueDatabase &) const
{
  return ReadFileAsString(":/shaders/blur.frag");
}

int BlurFilterNode::ShaderIterations() const
{
  // FIXME: Optimize if horiz_in or vert_in is disabled
  return 2;
}

NodeInput *BlurFilterNode::ShaderIterativeInput() const
{
  return texture_input_;
}

OLIVE_NAMESPACE_EXIT
