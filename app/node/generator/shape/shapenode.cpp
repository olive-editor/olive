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

#include "shapenode.h"

namespace olive {

#define super ShapeNodeBase

QString ShapeNode::kTypeInput = QStringLiteral("type_in");

ShapeNode::ShapeNode()
{
  PrependInput(kTypeInput, NodeValue::kCombo);
}

QString ShapeNode::Name() const
{
  return tr("Shape");
}

QString ShapeNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.shape");
}

QVector<Node::CategoryID> ShapeNode::Category() const
{
  return {kCategoryGenerator};
}

QString ShapeNode::Description() const
{
  return tr("Generate a 2D primitive shape.");
}

void ShapeNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTypeInput, tr("Type"));

  // Coordinate with Type enum
  SetComboBoxStrings(kTypeInput, {tr("Rectangle"), tr("Ellipse")});
}

ShaderCode ShapeNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)

  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/shape.frag")));
}

NodeValueTable ShapeNode::Value(const QString &output, NodeValueDatabase &value) const
{
  Q_UNUSED(output)

  ShaderJob job;

  job.InsertValue(this, kTypeInput, value);
  job.InsertValue(this, kPositionInput, value);
  job.InsertValue(this, kSizeInput, value);
  job.InsertValue(this, kColorInput, value);
  job.InsertValue(QStringLiteral("resolution_in"), value[QStringLiteral("global")].GetWithMeta(NodeValue::kVec2, QStringLiteral("resolution")));
  job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOn);

  NodeValueTable table = value.Merge();
  table.Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
  return table;
}

}
