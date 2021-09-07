/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

const QString BlurFilterNode::kTextureInput = QStringLiteral("tex_in");
const QString BlurFilterNode::kMethodInput = QStringLiteral("method_in");
const QString BlurFilterNode::kRadiusInput = QStringLiteral("radius_in");
const QString BlurFilterNode::kHorizInput = QStringLiteral("horiz_in");
const QString BlurFilterNode::kVertInput = QStringLiteral("vert_in");
const QString BlurFilterNode::kRepeatEdgePixelsInput = QStringLiteral("repeat_edge_pixels_in");

BlurFilterNode::BlurFilterNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kMethodInput, NodeValue::kCombo, 0);

  AddInput(kRadiusInput, NodeValue::kFloat, 10.0);
  SetInputProperty(kRadiusInput, QStringLiteral("min"), 0.0);

  AddInput(kHorizInput, NodeValue::kBoolean, true);

  AddInput(kVertInput, NodeValue::kBoolean, true);

  AddInput(kRepeatEdgePixelsInput, NodeValue::kBoolean, false);
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
  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kMethodInput, tr("Method"));
  SetComboBoxStrings(kMethodInput, { tr("Box"), tr("Gaussian") });
  SetInputName(kRadiusInput, tr("Radius"));
  SetInputName(kHorizInput, tr("Horizontal"));
  SetInputName(kVertInput, tr("Vertical"));
  SetInputName(kRepeatEdgePixelsInput, tr("Repeat Edge Pixels"));
}

ShaderCode BlurFilterNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/blur.frag"));
}

void BlurFilterNode::Value(const QString &output, const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  Q_UNUSED(output)

  ShaderJob job;

  job.InsertValue(value);
  job.InsertValue(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, globals.resolution(), this));

  // If there's no texture, no need to run an operation
  if (!job.GetValue(kTextureInput).data().isNull()) {

    // Check if radius > 0, and both "horiz" and/or "vert" are enabled
    if ((job.GetValue(kHorizInput).data().toBool() || job.GetValue(kVertInput).data().toBool())
        && job.GetValue(kRadiusInput).data().toDouble() > 0.0) {

      // Set iteration count to 2 if we're blurring both horizontally and vertically
      if (job.GetValue(kHorizInput).data().toBool() && job.GetValue(kVertInput).data().toBool()) {
        job.SetIterations(2, kTextureInput);
      }

      // If we're not repeating pixels, expect an alpha channel to appear
      if (!job.GetValue(kRepeatEdgePixelsInput).data().toBool()) {
        job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOn);
      }

      table->Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);

    } else {
      // If we're not performing the blur job, just push the texture
      table->Push(job.GetValue(kTextureInput));
    }

  }
}

}
