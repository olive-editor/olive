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

#include "flipdistortnode.h"

namespace olive {

const QString FlipDistortNode::kTextureInput = QStringLiteral("tex_in");
const QString FlipDistortNode::kHorizontalInput = QStringLiteral("horiz_in");
const QString FlipDistortNode::kVerticalInput = QStringLiteral("vert_in");

#define super Node

FlipDistortNode::FlipDistortNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kHorizontalInput, NodeValue::kBoolean, false);

  AddInput(kVerticalInput, NodeValue::kBoolean, false);

  SetFlag(kVideoEffect);
  SetEffectInput(kTextureInput);
}

QString FlipDistortNode::Name() const
{
  return tr("Flip");
}

QString FlipDistortNode::id() const
{
  return QStringLiteral("org.oliveeditor.Olive.flip");
}

QVector<Node::CategoryID> FlipDistortNode::Category() const
{
  return {kCategoryDistort};
}

QString FlipDistortNode::Description() const
{
  return tr("Flips an image horizontally or vertically");
}

void FlipDistortNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kHorizontalInput, tr("Horizontal"));
  SetInputName(kVerticalInput, tr("Vertical"));
}

ShaderCode FlipDistortNode::GetShaderCode(const ShaderRequest &request) const
{
  Q_UNUSED(request)
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/flip.frag"));
}

void FlipDistortNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  // If there's no texture, no need to run an operation
  if (TexturePtr tex = value[kTextureInput].toTexture()) {
    // Only run shader if at least one of flip or flop are selected
    if (value[kHorizontalInput].toBool() || value[kVerticalInput].toBool()) {
      table->Push(NodeValue::kTexture, tex->toJob(ShaderJob(value)), this);
    } else {
      // If we're not flipping or flopping just push the texture
      table->Push(value[kTextureInput]);
    }
  }
}

}
