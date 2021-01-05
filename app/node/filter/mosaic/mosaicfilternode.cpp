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

MosaicFilterNode::MosaicFilterNode()
{
  tex_input_ = new NodeInput(this, QStringLiteral("tex_in"), NodeValue::kTexture);

  horiz_input_ = new NodeInput(this, QStringLiteral("horiz_in"), NodeValue::kFloat, 32.0);
  horiz_input_->setProperty("min", 1.0);

  vert_input_ = new NodeInput(this, QStringLiteral("vert_in"), NodeValue::kFloat, 18.0);
  vert_input_->setProperty("min", 1.0);
}

void MosaicFilterNode::Retranslate()
{
  tex_input_->set_name(tr("Texture"));
  horiz_input_->set_name(tr("Horizontal"));
  vert_input_->set_name(tr("Vertical"));
}

NodeValueTable MosaicFilterNode::Value(NodeValueDatabase &value) const
{
  ShaderJob job;

  job.InsertValue(tex_input_, value);
  job.InsertValue(horiz_input_, value);
  job.InsertValue(vert_input_, value);

  // Mipmapping makes this look weird, so we just use bilinear for finding the color of each block
  job.SetInterpolation(tex_input_, Texture::kLinear);

  NodeValueTable table = value.Merge();

  if (!job.GetValue(tex_input_).data().isNull()) {
    TexturePtr texture = job.GetValue(tex_input_).data().value<TexturePtr>();

    if (texture
        && job.GetValue(horiz_input_).data().toInt() != texture->width()
        && job.GetValue(vert_input_).data().toInt() != texture->height()) {
      table.Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
    } else {
      table.Push(job.GetValue(tex_input_));
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
