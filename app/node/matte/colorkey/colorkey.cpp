/***
  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team
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

#include "colorkey.h"

namespace olive {

const QString ColorKeyNode::kTextureInput = QStringLiteral("tex_in");
const QString ColorKeyNode::kColorInput = QStringLiteral("color_in");
//const QString BlurFilterNode::kMethodInput = QStringLiteral("method_in");


ColorKeyNode::ColorKeyNode() {
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kColorInput, NodeValue::kColor, QVariant::fromValue(Color(0.0f, 1.0f, 0.0f, 1.0f)));

  // AddInput(kMethodInput, NodeValue::kCombo, 0);
}

Node *ColorKeyNode::copy() const
{
  return new ColorKeyNode();
}

QString ColorKeyNode::Name() const
{
  return tr("Color Key");
}

QString ColorKeyNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.colorkey");
}


QVector<Node::CategoryID> ColorKeyNode::Category() const
{
  return {kCategoryMatte};
}

QString ColorKeyNode::Description() const
{
  return tr("Color keyer");
}

void ColorKeyNode::Retranslate()
{
  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kColorInput, tr("Key Color"));
}

ShaderCode ColorKeyNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/colorkey.frag"));
}

void ColorKeyNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job;
  job.InsertValue(value);

  qDebug() << job.GetValue(kColorInput).data();
  table->Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
}

}  // namespace olive