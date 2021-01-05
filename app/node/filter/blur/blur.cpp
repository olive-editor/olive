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

#include "blur.h"

namespace olive {

BlurFilterNode::BlurFilterNode()
{
  texture_input_ = new NodeInput(this, QStringLiteral("tex_in"), NodeValue::kTexture);

  method_input_ = new NodeInput(this, QStringLiteral("method_in"), NodeValue::kCombo, 0);

  radius_input_ = new NodeInput(this, QStringLiteral("radius_in"), NodeValue::kFloat, 10.0f);
  radius_input_->setProperty("min", 0.0f);

  horiz_input_ = new NodeInput(this, QStringLiteral("horiz_in"), NodeValue::kBoolean, true);

  vert_input_ = new NodeInput(this, QStringLiteral("vert_in"), NodeValue::kBoolean, true);

  repeat_edge_pixels_input_ = new NodeInput(this, QStringLiteral("repeat_edge_pixels_in"), NodeValue::kBoolean, false);
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

QVector<Node::CategoryID> BlurFilterNode::Category() const
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

ShaderCode BlurFilterNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/blur.frag"));
}

NodeValueTable BlurFilterNode::Value(NodeValueDatabase &value) const
{
  ShaderJob job;

  job.InsertValue(texture_input_, value);
  job.InsertValue(method_input_, value);
  job.InsertValue(radius_input_, value);
  job.InsertValue(horiz_input_, value);
  job.InsertValue(vert_input_, value);
  job.InsertValue(repeat_edge_pixels_input_, value);
  job.InsertValue(QStringLiteral("resolution_in"),
                  NodeValue(NodeValue::kVec2, value[QStringLiteral("global")].Get(NodeValue::kVec2, QStringLiteral("resolution")), this));

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

      table.Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);

    } else {
      // If we're not performing the blur job, just push the texture
      table.Push(job.GetValue(texture_input_));
    }

  }

  return table;
}

}
