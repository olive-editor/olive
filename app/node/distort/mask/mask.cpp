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

MaskDistortNode::MaskDistortNode()
{
  // Mask should always be (1.0, 1.0, 1.0) for multiply to work correctly
  SetInputFlags(kColorInput, InputFlags(GetInputFlags(kColorInput) | kInputFlagHidden));

  AddInput(kFeatherInput, NodeValue::kFloat, 0.0);
  SetInputProperty(kFeatherInput, QStringLiteral("min"), 0.0);
}

ShaderCode MaskDistortNode::GetShaderCode(const ShaderRequest &request) const
{
  if (request.id == QStringLiteral("mrg")) {
    return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/multiply.frag")));
  } else if (request.id == QStringLiteral("feather")) {
    return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/blur.frag")));
  } else {
    return super::GetShaderCode(request);
  }
}

void MaskDistortNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kBaseInput, tr("Texture"));
  SetInputName(kFeatherInput, tr("Feather"));
}

void MaskDistortNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job = GetGenerateJob(value);

  if (value[kBaseInput].toTexture()) {
    // Push as merge node
    ShaderJob merge;

    merge.SetShaderID(QStringLiteral("mrg"));
    merge.Insert(QStringLiteral("tex_a"), value[kBaseInput]);

    if (value[kFeatherInput].toDouble() > 0.0) {
      // Nest a blur shader in there too
      ShaderJob feather;

      feather.SetShaderID(QStringLiteral("feather"));
      feather.Insert(BlurFilterNode::kTextureInput, NodeValue(NodeValue::kTexture, job, this));
      feather.Insert(BlurFilterNode::kMethodInput, NodeValue(NodeValue::kInt, int(BlurFilterNode::kGaussian), this));
      feather.Insert(BlurFilterNode::kHorizInput, NodeValue(NodeValue::kBoolean, true, this));
      feather.Insert(BlurFilterNode::kVertInput, NodeValue(NodeValue::kBoolean, true, this));
      feather.Insert(BlurFilterNode::kRepeatEdgePixelsInput, NodeValue(NodeValue::kBoolean, true, this));
      feather.Insert(BlurFilterNode::kRadiusInput, NodeValue(NodeValue::kFloat, value[kFeatherInput].toDouble(), this));
      feather.SetIterations(2, BlurFilterNode::kTextureInput);
      feather.Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, globals.resolution(), this));

      merge.Insert(QStringLiteral("tex_b"), NodeValue(NodeValue::kTexture, feather, this));
    } else {
      merge.Insert(QStringLiteral("tex_b"), NodeValue(NodeValue::kTexture, job, this));
    }

    table->Push(NodeValue::kTexture, QVariant::fromValue(merge), this);
  }
}

}
