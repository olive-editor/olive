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

#include "merge.h"

namespace olive {

MergeNode::MergeNode()
{
  base_in_ = new NodeInput("base_in", NodeParam::kTexture);
  AddInput(base_in_);

  blend_in_ = new NodeInput("blend_in", NodeParam::kTexture);
  AddInput(blend_in_);
}

Node *MergeNode::copy() const
{
  return new MergeNode();
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
  base_in_->set_name(tr("Base"));
  blend_in_->set_name(tr("Blend"));
}

ShaderCode MergeNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)

  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/alphaover.frag"), QString());
}

NodeValueTable MergeNode::Value(NodeValueDatabase &value) const
{
  ShaderJob job;
  job.InsertValue(base_in_, value);
  job.InsertValue(blend_in_, value);

  NodeValueTable table = value.Merge();

  TexturePtr base_tex = job.GetValue(base_in_).data.value<TexturePtr>();
  TexturePtr blend_tex = job.GetValue(blend_in_).data.value<TexturePtr>();

  if (base_tex || blend_tex) {
    if (!base_tex || (blend_tex && blend_tex->channel_count() < VideoParams::kRGBAChannelCount)) {
      // We only have a blend texture or the blend texture is RGB only, no need to alpha over
      table.Push(job.GetValue(blend_in_), this);
    } else if (!blend_tex) {
      // We only have a base texture, no need to alpha over
      table.Push(job.GetValue(base_in_), this);
    } else {
      // We have both textures, push the job
      table.Push(NodeParam::kShaderJob, QVariant::fromValue(job), this);
    }
  }

  return table;
}

NodeInput *MergeNode::base_in() const
{
  return base_in_;
}

NodeInput *MergeNode::blend_in() const
{
  return blend_in_;
}

void MergeNode::Hash(QCryptographicHash &hash, const rational &time) const
{
  if (base_in_->is_connected()) {
    base_in_->get_connected_node()->Hash(hash, time);
  }

  if (blend_in_->is_connected()) {
    blend_in_->get_connected_node()->Hash(hash, time);
  }
}

}
