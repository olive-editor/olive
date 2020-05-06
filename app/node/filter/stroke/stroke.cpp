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

#include "stroke.h"

#include "render/color.h"

OLIVE_NAMESPACE_ENTER

StrokeFilterNode::StrokeFilterNode()
{
  tex_input_ = new NodeInput("tex_in", NodeParam::kTexture);
  AddInput(tex_input_);

  color_input_ = new NodeInput("color_in",
                               NodeParam::kColor,
                               QVariant::fromValue(Color(1.0f, 1.0f, 1.0f, 1.0f)));
  AddInput(color_input_);

  radius_input_ = new NodeInput("radius_in", NodeParam::kFloat, 10.0f);
  radius_input_->set_property("min", 0.0f);
  AddInput(radius_input_);

  opacity_input_ = new NodeInput("opacity_in", NodeParam::kFloat, 1.0f);
  opacity_input_->set_property("view", "percent");
  opacity_input_->set_property("min", 0.0f);
  opacity_input_->set_property("max", 1.0f);
  AddInput(opacity_input_);

  inner_input_ = new NodeInput("inner_in", NodeParam::kBoolean, false);
  AddInput(inner_input_);
}

Node *StrokeFilterNode::copy() const
{
  return new StrokeFilterNode();
}

QString StrokeFilterNode::Name() const
{
  return tr("Stroke");
}

QString StrokeFilterNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.stroke");
}

QList<Node::CategoryID> StrokeFilterNode::Category() const
{
  return {kCategoryFilter};
}

QString StrokeFilterNode::Description() const
{
  return tr("Creates a stroke outline around an image.");
}

void StrokeFilterNode::Retranslate()
{
  tex_input_->set_name(tr("Input"));
  color_input_->set_name(tr("Color"));
  radius_input_->set_name(tr("Radius"));
  opacity_input_->set_name(tr("Opacity"));
  inner_input_->set_name(tr("Inner"));
}

Node::Capabilities StrokeFilterNode::GetCapabilities(const NodeValueDatabase &) const
{
  return kShader;
}

QString StrokeFilterNode::ShaderFragmentCode(const NodeValueDatabase &) const
{
  return ReadFileAsString(":/shaders/stroke.frag");
}

OLIVE_NAMESPACE_EXIT
