/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "merge.h"

namespace olive {

const QString MergeNode::kBaseIn = QStringLiteral("base_in");
const QString MergeNode::kBlendIn = QStringLiteral("blend_in");

#define super Node

MergeNode::MergeNode()
{
  AddInput(kBaseIn, TYPE_TEXTURE, kInputFlagNotKeyframable);

  AddInput(kBlendIn, TYPE_TEXTURE, kInputFlagNotKeyframable);

  SetFlag(kDontShowInParamView);
}

QString MergeNode::Name() const
{
  return tr("Merge");
}

QString MergeNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.merge");
}

QVector<Node::CategoryID> MergeNode::Category() const
{
  return {kCategoryMath};
}

QString MergeNode::Description() const
{
  return tr("Merge two textures together.");
}

void MergeNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kBaseIn, tr("Base"));

  SetInputName(kBlendIn, tr("Blend"));
}

ShaderCode MergeNode::GetShaderCode(const QString &id)
{
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/alphaover.frag"));
}

value_t MergeNode::Value(const ValueParams &p) const
{
  value_t base_val = GetInputValue(p, kBaseIn);
  value_t blend_val = GetInputValue(p, kBlendIn);

  TexturePtr base_tex = base_val.toTexture();
  TexturePtr blend_tex = blend_val.toTexture();

  if (base_tex || blend_tex) {
    if (!base_tex || (blend_tex && blend_tex->channel_count() < VideoParams::kRGBAChannelCount)) {
      // We only have a blend texture or the blend texture is RGB only, no need to alpha over
      return blend_val;
    } else if (!blend_tex) {
      // We only have a base texture, no need to alpha over
      return base_val;
    } else {
      return base_tex->toJob(CreateShaderJob(p, GetShaderCode));
    }
  }

  return value_t();
}

}
