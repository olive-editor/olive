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

#include "mask.h"

namespace olive {

#define super PolygonGenerator

MaskDistortNode::MaskDistortNode()
{
  // Mask should always be (1.0, 1.0, 1.0) for multiply to work correctly
  SetInputFlags(kColorInput, InputFlags(GetInputFlags(kColorInput) | kInputFlagHidden));
}

ShaderCode MaskDistortNode::GetShaderCode(const ShaderRequest &request) const
{
  Q_UNUSED(request)
  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/multiply.frag")));
}

void MaskDistortNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kBaseInput, tr("Texture"));
}

void MaskDistortNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  GenerateJob job = GetGenerateJob(value);

  if (!value[kBaseInput].data().isNull()) {
    // Push as merge node
    ShaderJob merge;

    merge.SetShaderID(QStringLiteral("mrg"));
    merge.InsertValue(QStringLiteral("tex_a"), value[kBaseInput]);
    merge.InsertValue(QStringLiteral("tex_b"), NodeValue(NodeValue::kTexture, QVariant::fromValue(job), this));

    table->Push(NodeValue::kTexture, QVariant::fromValue(merge), this);
  }
}

}
