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

void ShapeNode::Value(const QString &output, const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  Q_UNUSED(output)

  ShaderJob job;

  job.InsertValue(value);
  job.InsertValue(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, globals.resolution(), this));
  job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOn);

  table->Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
}

}
