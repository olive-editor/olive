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

#include "colordifferencekey.h"

namespace olive {

const QString ColorDifferenceKeyNode::kTextureInput = QStringLiteral("tex_in");
const QString ColorDifferenceKeyNode::kGarbageMatteInput = QStringLiteral("garbage_in");
const QString ColorDifferenceKeyNode::kCoreMatteInput = QStringLiteral("core_in");
const QString ColorDifferenceKeyNode::kColorInput = QStringLiteral("color_in");
const QString ColorDifferenceKeyNode::kShadowsInput = QStringLiteral("shadows_in");
const QString ColorDifferenceKeyNode::kHighlightsInput = QStringLiteral("highlights_in");
const QString ColorDifferenceKeyNode::kMaskOnlyInput = QStringLiteral("mask_only_in");

ColorDifferenceKeyNode::ColorDifferenceKeyNode() {
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kGarbageMatteInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kCoreMatteInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kColorInput, NodeValue::kCombo, 0);

  AddInput(kHighlightsInput, NodeValue::kFloat, 100.0f);
  SetInputProperty(kHighlightsInput, QStringLiteral("min"), 0.0);

  AddInput(kShadowsInput, NodeValue::kFloat, 100.0f);
  SetInputProperty(kShadowsInput, QStringLiteral("min"), 0.0);

  AddInput(kMaskOnlyInput, NodeValue::kBoolean, false);
}

Node *ColorDifferenceKeyNode::copy() const
{
  return new ColorDifferenceKeyNode();
}

QString ColorDifferenceKeyNode::Name() const
{
  return tr("Color Difference Key");
}

QString ColorDifferenceKeyNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.colordifferencekey");
}

QVector<Node::CategoryID> ColorDifferenceKeyNode::Category() const
{
  return {kCategoryKeying};
}

QString ColorDifferenceKeyNode::Description() const
{
  return tr("A simple color key based on the distance of one color from other colors.");
}

void ColorDifferenceKeyNode::Retranslate()
{
  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kGarbageMatteInput, tr("Garbage Matte"));
  SetInputName(kCoreMatteInput, tr("Core Matte"));
  SetInputName(kColorInput, tr("Key Color"));
  SetComboBoxStrings(kColorInput, {tr("Green"), tr("Blue")});
  SetInputName(kShadowsInput, tr("Shadows"));
  SetInputName(kHighlightsInput, tr("Highlights"));
  SetInputName(kMaskOnlyInput, tr("Show Mask Only"));
}

ShaderCode ColorDifferenceKeyNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/colordifferencekey.frag"));
}

void ColorDifferenceKeyNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job;
  job.InsertValue(value);
  job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOn);

  // If there's no texture, no need to run an operation
  if (!job.GetValue(kTextureInput).data().isNull()) {
    table->Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
  }
}

}  // namespace olive
