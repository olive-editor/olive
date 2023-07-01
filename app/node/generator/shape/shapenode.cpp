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

#include "shapenode.h"

namespace olive {

#define super ShapeNodeBase

QString ShapeNode::kTypeInput = QStringLiteral("type_in");
QString ShapeNode::kRadiusInput = QStringLiteral("radius_in");

ShapeNode::ShapeNode()
{
  PrependInput(kTypeInput, NodeValue::kCombo);

  AddInput(kRadiusInput, NodeValue::kFloat, 20.0);
  SetInputProperty(kRadiusInput, QStringLiteral("min"), 0.0);
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
  SetInputName(kRadiusInput, tr("Radius"));

  // Coordinate with Type enum
  SetComboBoxStrings(kTypeInput, {tr("Rectangle"), tr("Ellipse"), tr("Rounded Rectangle")});
}

ShaderCode ShapeNode::GetShaderCode(const ShaderRequest &request) const
{
  if (request.id == QStringLiteral("shape")) {
    return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/shape.frag")));
  } else {
    return super::GetShaderCode(request);
  }
}

void ShapeNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  TexturePtr base = value[kBaseInput].toTexture();

  ShaderJob job(value);

  job.Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, base ? base->virtual_resolution() : globals.square_resolution(), this));
  job.SetShaderID(QStringLiteral("shape"));

  PushMergableJob(value, Texture::Job(base ? base->params() : globals.vparams(), job), table);
}

void ShapeNode::InputValueChangedEvent(const QString &input, int element)
{
  if (input == kTypeInput) {
    SetInputFlag(kRadiusInput, kInputFlagHidden, (GetStandardValue(kTypeInput).toInt() != kRoundedRectangle));
  }
  super::InputValueChangedEvent(input, element);
}

}
