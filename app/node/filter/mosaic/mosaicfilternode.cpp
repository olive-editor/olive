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

  SetFlags(kVideoEffect);
  SetEffectInput(kTextureInput);
}

void MosaicFilterNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextureInput, tr("Texture"));
  SetInputName(kHorizInput, tr("Horizontal"));
  SetInputName(kVertInput, tr("Vertical"));
}

void MosaicFilterNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job;

  job.Insert(value);

  // Mipmapping makes this look weird, so we just use bilinear for finding the color of each block
  job.SetInterpolation(kTextureInput, Texture::kLinear);

  if (job.Get(kTextureInput).toTexture()) {
    TexturePtr texture = job.Get(kTextureInput).toTexture();

    if (texture
        && job.Get(kHorizInput).toInt() != texture->width()
        && job.Get(kVertInput).toInt() != texture->height()) {
      table->Push(NodeValue::kTexture, QVariant::fromValue(job), this);
    } else {
      table->Push(job.Get(kTextureInput));
    }
  }
}

ShaderCode MosaicFilterNode::GetShaderCode(const ShaderRequest &request) const
{
  Q_UNUSED(request)

  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/mosaic.frag"));
}

}
