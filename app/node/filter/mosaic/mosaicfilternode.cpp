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

#include "mosaicfilternode.h"

namespace olive {

const QString MosaicFilterNode::kTextureInput = QStringLiteral("tex_in");
const QString MosaicFilterNode::kHorizInput = QStringLiteral("horiz_in");
const QString MosaicFilterNode::kVertInput = QStringLiteral("vert_in");

MosaicFilterNode::MosaicFilterNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kHorizInput, NodeValue::kFloat, 32.0);
  SetInputProperty(kHorizInput, QStringLiteral("min"), 1.0);

  AddInput(kVertInput, NodeValue::kFloat, 18.0);
  SetInputProperty(kVertInput, QStringLiteral("min"), 1.0);
}

void MosaicFilterNode::Retranslate()
{
  SetInputName(kTextureInput, tr("Texture"));
  SetInputName(kHorizInput, tr("Horizontal"));
  SetInputName(kVertInput, tr("Vertical"));
}

NodeValueTable MosaicFilterNode::Value(const QString &output, NodeValueDatabase &value) const
{
  Q_UNUSED(output)

  ShaderJob job;

  job.InsertValue(this, kTextureInput, value);
  job.InsertValue(this, kHorizInput, value);
  job.InsertValue(this, kVertInput, value);

  // Mipmapping makes this look weird, so we just use bilinear for finding the color of each block
  job.SetInterpolation(kTextureInput, Texture::kLinear);

  NodeValueTable table = value.Merge();

  if (!job.GetValue(kTextureInput).data().isNull()) {
    TexturePtr texture = job.GetValue(kTextureInput).data().value<TexturePtr>();

    if (texture
        && job.GetValue(kHorizInput).data().toInt() != texture->width()
        && job.GetValue(kVertInput).data().toInt() != texture->height()) {
      table.Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
    } else {
      table.Push(job.GetValue(kTextureInput));
    }
  }

  return table;
}

ShaderCode MosaicFilterNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)

  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/mosaic.frag"));
}

}
