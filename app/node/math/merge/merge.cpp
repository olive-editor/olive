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

const QString MergeNode::kBaseIn = QStringLiteral("base_in");
const QString MergeNode::kBlendIn = QStringLiteral("blend_in");

MergeNode::MergeNode()
{
  AddInput(kBaseIn, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kBlendIn, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));
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
  SetInputName(kBaseIn, tr("Base"));

  SetInputName(kBlendIn, tr("Blend"));
}

ShaderCode MergeNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)

  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/alphaover.frag"));
}

NodeValueTable MergeNode::Value(const QString &output, NodeValueDatabase &value) const
{
  Q_UNUSED(output)

  ShaderJob job;
  job.InsertValue(this, kBaseIn, value);
  job.InsertValue(this, kBlendIn, value);

  NodeValueTable table = value.Merge();

  TexturePtr base_tex = job.GetValue(kBaseIn).data().value<TexturePtr>();
  TexturePtr blend_tex = job.GetValue(kBlendIn).data().value<TexturePtr>();

  if (base_tex || blend_tex) {
    if (!base_tex || (blend_tex && blend_tex->channel_count() < VideoParams::kRGBAChannelCount)) {
      // We only have a blend texture or the blend texture is RGB only, no need to alpha over
      table.Push(job.GetValue(kBlendIn));
    } else if (!blend_tex) {
      // We only have a base texture, no need to alpha over
      table.Push(job.GetValue(kBaseIn));
    } else {
      // We have both textures, push the job
      table.Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
    }
  }

  return table;
}

void MergeNode::Hash(QCryptographicHash &hash, const rational &time) const
{
  // We do some hash optimization here. If only one of the inputs is connected, this node
  // functions as a passthrough so there's no alteration to the hash. The same is true if the
  // connected node happens to return nothing (a gap for instance). Therefore we only add our
  // fingerprint if the base AND the blend change the hash. Otherwise, we assume it's a passthrough.

  QByteArray current_result = hash.result();

  bool base_changed_hash = false;
  bool blend_changed_hash = false;

  if (IsInputConnected(kBaseIn)) {
    GetConnectedNode(kBaseIn)->Hash(hash, time);

    QByteArray post_base_hash = hash.result();
    base_changed_hash = (post_base_hash != current_result);
    current_result = post_base_hash;
  }

  if(IsInputConnected(kBlendIn)) {
    GetConnectedNode(kBlendIn)->Hash(hash, time);

    blend_changed_hash = (hash.result() != current_result);
  }

  if (base_changed_hash && blend_changed_hash) {
    // Something changed, so we'll add our fingerprint
    hash.addData(id().toUtf8());
  }
}

}
