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
  base_in_ = new NodeInput(this, QStringLiteral("base_in"), NodeValue::kTexture);

  blend_in_ = new NodeInput(this, QStringLiteral("blend_in"), NodeValue::kTexture);
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

  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/alphaover.frag"));
}

NodeValueTable MergeNode::Value(NodeValueDatabase &value) const
{
  ShaderJob job;
  job.InsertValue(base_in_, value);
  job.InsertValue(blend_in_, value);

  NodeValueTable table = value.Merge();

  TexturePtr base_tex = job.GetValue(base_in_).data().value<TexturePtr>();
  TexturePtr blend_tex = job.GetValue(blend_in_).data().value<TexturePtr>();

  if (base_tex || blend_tex) {
    if (!base_tex || (blend_tex && blend_tex->channel_count() < VideoParams::kRGBAChannelCount)) {
      // We only have a blend texture or the blend texture is RGB only, no need to alpha over
      table.Push(job.GetValue(blend_in_));
    } else if (!blend_tex) {
      // We only have a base texture, no need to alpha over
      table.Push(job.GetValue(base_in_));
    } else {
      // We have both textures, push the job
      table.Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
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
  // If only one of these is connected, the merge is a no-op, so we only leave a fingerprint if
  // both are connected
  if (base_in_->IsConnected() && blend_in_->IsConnected()) {
    // Leave fingerprint of merge node
    hash.addData(id().toUtf8());
  }

  if (base_in_->IsConnected()) {
    base_in_->GetConnectedNode()->Hash(hash, time);
  }

  if (blend_in_->IsConnected()) {
    blend_in_->GetConnectedNode()->Hash(hash, time);
  }
}

}
