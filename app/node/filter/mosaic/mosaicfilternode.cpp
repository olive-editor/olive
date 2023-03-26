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

#include "mosaicfilternode.h"

namespace olive {

const QString MosaicFilterNode::kTextureInput = QStringLiteral("tex_in");
const QString MosaicFilterNode::kHorizInput = QStringLiteral("horiz_in");
const QString MosaicFilterNode::kVertInput = QStringLiteral("vert_in");

#define super Node

MosaicFilterNode::MosaicFilterNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kHorizInput, NodeValue::kFloat, 32.0);
  SetInputProperty(kHorizInput, QStringLiteral("min"), 1.0);

  AddInput(kVertInput, NodeValue::kFloat, 18.0);
  SetInputProperty(kVertInput, QStringLiteral("min"), 1.0);

  SetFlag(kVideoEffect);
  SetEffectInput(kTextureInput);
}

void MosaicFilterNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextureInput, tr("Texture"));
  SetInputName(kHorizInput, tr("Horizontal"));
  SetInputName(kVertInput, tr("Vertical"));
}

NodeValue MosaicFilterNode::Value(const ValueParams &p) const
{
  NodeValue tex_meta = GetInputValue(p, kTextureInput);

  if (TexturePtr texture = tex_meta.toTexture()) {
    if (texture
        && GetInputValue(p, kHorizInput).toInt() != texture->width()
        && GetInputValue(p, kVertInput).toInt() != texture->height()) {
      ShaderJob job = CreateJob<ShaderJob>(p);

      // Mipmapping makes this look weird, so we just use bilinear for finding the color of each block
      job.SetInterpolation(kTextureInput, Texture::kLinear);

      return NodeValue(NodeValue::kTexture, texture->toJob(job), this);
    }
  }

  return tex_meta;
}

ShaderCode MosaicFilterNode::GetShaderCode(const ShaderRequest &request) const
{
  Q_UNUSED(request)

  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/mosaic.frag"));
}

}
