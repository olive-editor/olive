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

#include "color.h"

OLIVE_NAMESPACE_ENTER

ColorFilterNode::ColorFilterNode()
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

Node *ColorFilterNode::copy() const
{
  return new ColorFilterNode();
}

QString ColorFilterNode::Name() const
{
  return tr("Color");
}

QString ColorFilterNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.color");
}

QList<Node::CategoryID> ColorFilterNode::Category() const
{
  return {kCategoryFilter};
}

QString ColorFilterNode::Description() const
{
  return tr("Blurs an image.");
}

void ColorFilterNode::Retranslate()
{
  texture_input_->set_name(tr("Input"));
  method_input_->set_name(tr("Method"));
  method_input_->set_combobox_strings({ tr("Box"), tr("Gaussian") });
  radius_input_->set_name(tr("Radius"));
  horiz_input_->set_name(tr("Horizontal"));
  vert_input_->set_name(tr("Vertical"));
  repeat_edge_pixels_input_->set_name(tr("Repeat Edge Pixels"));
}

ShaderCode ColorFilterNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)
  return ShaderCode(ReadFileAsString(":/shaders/blur.frag"), QString());
}

NodeValueTable ColorFilterNode::Value(NodeValueDatabase &value) const
{
  ShaderJob job;

  job.InsertValue(texture_input_, value);
  job.InsertValue(method_input_, value);
  job.InsertValue(radius_input_, value);
  job.InsertValue(horiz_input_, value);
  job.InsertValue(vert_input_, value);
  job.InsertValue(repeat_edge_pixels_input_, value);

  NodeValueTable table = value.Merge();

  // If there's no texture, no need to run an operation
  if (!job.GetValue(texture_input_).data().isNull()) {

    // Check if radius > 0, and both "horiz" and/or "vert" are enabled
    if ((job.GetValue(horiz_input_).data().toBool() || job.GetValue(vert_input_).data().toBool())
        && job.GetValue(radius_input_).data().toDouble() > 0.0) {

      // Set iteration count to 2 if we're blurring both horizontally and vertically
      if (job.GetValue(horiz_input_).data().toBool() && job.GetValue(vert_input_).data().toBool()) {
        job.SetIterations(2, texture_input_);
      }

      // If we're not repeating pixels, expect an alpha channel to appear
      if (!job.GetValue(repeat_edge_pixels_input_).data().toBool()) {
        job.SetAlphaChannelRequired(true);
      }

      table.Push(NodeParam::kShaderJob, QVariant::fromValue(job), this);

    } else {
      // If we're not performing the blur job, just push the texture
      table.Push(job.GetValue(texture_input_));
    }

  }

  return table;
}

OLIVE_NAMESPACE_EXIT
