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

#include "flipflopdistortnode.h"

namespace olive {

const QString FlipFlopDistortNode::kTextureInput = QStringLiteral("tex_in");
const QString FlipFlopDistortNode::kHorizontalInput = QStringLiteral("horiz_in");
const QString FlipFlopDistortNode::kVerticalInput = QStringLiteral("vert_in");

FlipFlopDistortNode::FlipFlopDistortNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kHorizontalInput, NodeValue::kBoolean, false);

  AddInput(kVerticalInput, NodeValue::kBoolean, false);
}

Node* FlipFlopDistortNode::copy() const
{
  return new FlipFlopDistortNode();
}

QString FlipFlopDistortNode::Name() const
{
  return tr("Flip/Flop");
}

QString FlipFlopDistortNode::id() const
{
  return QStringLiteral("org.oliveeditor.Olive.flipflop");
}

QVector<Node::CategoryID> FlipFlopDistortNode::Category() const
{
  return {kCategoryDistort};
}

QString FlipFlopDistortNode::Description() const
{
  return tr("Flips an image horizontally. Flops an image vertically");
}

void FlipFlopDistortNode::Retranslate()
{
  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kHorizontalInput, tr("Horizontal"));
  SetInputName(kVerticalInput, tr("Vertical"));
}

ShaderCode FlipFlopDistortNode::GetShaderCode(const QString& shader_id) const
{
  Q_UNUSED(shader_id)
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/flipflop.frag"));
}

void FlipFlopDistortNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job;

  job.InsertValue(value);

  // If there's no texture, no need to run an operation
  if (!job.GetValue(kTextureInput).data().isNull()) {
    // Only run shader if at least one of flip or flop are selected
    if (job.GetValue(kHorizontalInput).data().toBool() || job.GetValue(kVerticalInput).data().toBool()) {
      table->Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
    } else {
    // If we're not flipping or flopping just push the texture
    table->Push(job.GetValue(kTextureInput));
    }
  }

}

}
