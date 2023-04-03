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

#define super Node

ColorDifferenceKeyNode::ColorDifferenceKeyNode()
{
  AddInput(kTextureInput, TYPE_TEXTURE, kInputFlagNotKeyframable);

  AddInput(kGarbageMatteInput, TYPE_TEXTURE, kInputFlagNotKeyframable);

  AddInput(kCoreMatteInput, TYPE_TEXTURE, kInputFlagNotKeyframable);

  AddInput(kColorInput, TYPE_COMBO, 0);

  AddInput(kHighlightsInput, TYPE_DOUBLE, 1.0f);
  SetInputProperty(kHighlightsInput, QStringLiteral("min"), 0.0);
  SetInputProperty(kHighlightsInput, QStringLiteral("base"), 0.01);

  AddInput(kShadowsInput, TYPE_DOUBLE, 1.0f);
  SetInputProperty(kShadowsInput, QStringLiteral("min"), 0.0);
  SetInputProperty(kShadowsInput, QStringLiteral("base"), 0.01);

  AddInput(kMaskOnlyInput, TYPE_BOOL, false);

  SetFlag(kVideoEffect);
  SetEffectInput(kTextureInput);
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
  super::Retranslate();

  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kGarbageMatteInput, tr("Garbage Matte"));
  SetInputName(kCoreMatteInput, tr("Core Matte"));
  SetInputName(kColorInput, tr("Key Color"));
  SetComboBoxStrings(kColorInput, {tr("Green"), tr("Blue")});
  SetInputName(kShadowsInput, tr("Shadows"));
  SetInputName(kHighlightsInput, tr("Highlights"));
  SetInputName(kMaskOnlyInput, tr("Show Mask Only"));
}

ShaderCode ColorDifferenceKeyNode::GetShaderCode(const QString &id)
{
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/colordifferencekey.frag"));
}

value_t ColorDifferenceKeyNode::Value(const ValueParams &p) const
{
  // If there's no texture, no need to run an operation
  value_t tex_meta = GetInputValue(p, kTextureInput);

  if (TexturePtr tex = tex_meta.toTexture()) {
    ShaderJob job = CreateShaderJob(p, GetShaderCode);
    return tex->toJob(job);
  }

  return tex_meta;
}

}  // namespace olive
