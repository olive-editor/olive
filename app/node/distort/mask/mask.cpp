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

#include "mask.h"

#include "node/filter/blur/blur.h"

namespace olive {

#define super PolygonGenerator

const QString MaskDistortNode::kFeatherInput = QStringLiteral("feather_in");
const QString MaskDistortNode::kInvertInput = QStringLiteral("invert_in");

MaskDistortNode::MaskDistortNode()
{
  // Mask should always be (1.0, 1.0, 1.0) for multiply to work correctly
  SetInputFlag(kColorInput, kInputFlagHidden);

  AddInput(kInvertInput, NodeValue::kBoolean, false);

  AddInput(kFeatherInput, NodeValue::kFloat, 0.0);
  SetInputProperty(kFeatherInput, QStringLiteral("min"), 0.0);
}

ShaderCode MaskDistortNode::GetShaderCode(const ShaderRequest &request) const
{
  if (request.id == QStringLiteral("mrg")) {
    return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/multiply.frag")));
  } else if (request.id == QStringLiteral("feather")) {
    return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/blur.frag")));
  } else if (request.id == QStringLiteral("invert")) {
    return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/invertrgba.frag")));
  } else {
    return super::GetShaderCode(request);
  }
}

void MaskDistortNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kBaseInput, tr("Texture"));
  SetInputName(kInvertInput, tr("Invert"));
  SetInputName(kFeatherInput, tr("Feather"));
}

NodeValue MaskDistortNode::Value(const ValueParams &p) const
{
  TexturePtr texture = GetInputValue(p, kBaseInput).toTexture();

  VideoParams job_params = texture ? texture->params() : p.vparams();
  NodeValue job(NodeValue::kTexture, Texture::Job(job_params, GetGenerateJob(p, job_params)), this);

  if (GetInputValue(p, kInvertInput).toBool()) {
    ShaderJob invert;
    invert.SetShaderID(QStringLiteral("invert"));
    invert.Insert(QStringLiteral("tex_in"), job);
    job.set_value(Texture::Job(job_params, invert));
  }

  if (texture) {
    // Push as merge node
    ShaderJob merge;

    merge.SetShaderID(QStringLiteral("mrg"));
    merge.Insert(QStringLiteral("tex_a"), GetInputValue(p, kBaseInput));

    if (GetInputValue(p, kFeatherInput).toDouble() > 0.0) {
      // Nest a blur shader in there too
      ShaderJob feather;

      feather.SetShaderID(QStringLiteral("feather"));
      feather.Insert(BlurFilterNode::kTextureInput, job);
      feather.Insert(BlurFilterNode::kMethodInput, NodeValue(NodeValue::kInt, int(BlurFilterNode::kGaussian), this));
      feather.Insert(BlurFilterNode::kHorizInput, NodeValue(NodeValue::kBoolean, true, this));
      feather.Insert(BlurFilterNode::kVertInput, NodeValue(NodeValue::kBoolean, true, this));
      feather.Insert(BlurFilterNode::kRepeatEdgePixelsInput, NodeValue(NodeValue::kBoolean, true, this));
      feather.Insert(BlurFilterNode::kRadiusInput, NodeValue(NodeValue::kFloat, GetInputValue(p, kFeatherInput).toDouble(), this));
      feather.SetIterations(2, BlurFilterNode::kTextureInput);
      feather.Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, texture ? texture->virtual_resolution() : p.square_resolution(), this));

      merge.Insert(QStringLiteral("tex_b"), NodeValue(NodeValue::kTexture, Texture::Job(job_params, feather), this));
    } else {
      merge.Insert(QStringLiteral("tex_b"), job);
    }

    return NodeValue(NodeValue::kTexture, Texture::Job(job_params, merge), this);
  } else {
    return job;
  }
}

}
