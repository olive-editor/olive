/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "node/traverser.h"

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
      if (base_tex->channel_count() < VideoParams::kRGBAChannelCount) {
        // Base has no alpha, therefore this merge operation will not add an alpha channel
        job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOff);
      }

      table.Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
    }
  }

  return table;
}

void MergeNode::Hash(const QString &output, QCryptographicHash &hash, const rational &time, const VideoParams &video_params) const
{
  NodeTraverser traverser;
  traverser.SetCacheVideoParams(video_params);

  NodeValueDatabase db = traverser.GenerateDatabase(this, output, TimeRange(time, time+video_params.frame_rate_as_time_base()));

  TexturePtr base_tex = db[kBaseIn].Get(NodeValue::kTexture).value<TexturePtr>();
  TexturePtr blend_tex = db[kBlendIn].Get(NodeValue::kTexture).value<TexturePtr>();

  if (base_tex || blend_tex) {
    bool passthrough_base = !blend_tex;
    bool passthrough_blend = !base_tex || (blend_tex && blend_tex->channel_count() < VideoParams::kRGBAChannelCount);

    if (!passthrough_base && !passthrough_blend) {
      // This merge will actually do something so we add a fingerprint
      hash.addData(id().toUtf8());
    }

    if (!passthrough_base) {
      NodeOutput blend_output = GetConnectedOutput(kBlendIn);
      blend_output.node()->Hash(blend_output.output(), hash, time, video_params);
    }

    if (!passthrough_blend) {
      NodeOutput base_output = GetConnectedOutput(kBaseIn);
      base_output.node()->Hash(base_output.output(), hash, time, video_params);
    }

    Q_ASSERT(!passthrough_base || !passthrough_blend);
  }
}

}
