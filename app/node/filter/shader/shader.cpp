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

#include "shader.h"

#include <QRegularExpression>
#include <QFileInfo>

#include "shaderinputsparser.h"

namespace olive {

const QString ShaderFilterNode::kShaderCode = QStringLiteral("source");


ShaderFilterNode::ShaderFilterNode()
{
  // Full code of the shader. Inputs to be exposed are defined within the shader code
  // with mark-up comments.
  AddInput(kShaderCode, NodeValue::kText, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  // mark this text input as code, so it will be edited with code editor and not
  // with a simple textbox
  SetInputProperty( kShaderCode, QStringLiteral("is_shader_code"), true);
}

Node *ShaderFilterNode::copy() const
{
  ShaderFilterNode * new_node = new ShaderFilterNode();
  new_node->shader_code_ = this->shader_code_;
  new_node->input_list_ = this->input_list_;
  new_node->onShaderCodeChanged();

  return new_node;
}

QString ShaderFilterNode::Name() const
{
  return QString("shader");
}

QString ShaderFilterNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.shader");
}

QVector<Node::CategoryID> ShaderFilterNode::Category() const
{
  return {kCategoryFilter};
}

void olive::ShaderFilterNode::onShaderCodeChanged()
{
  qDebug() << "parsing shader code";

  // pre-remove all inputs
  for (QString oldInput : input_list_)
  {
    RemoveInput(oldInput);
  }
  input_list_.clear();

  parseShaderCode();
}

void ShaderFilterNode::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(element)

  if (input == kShaderCode)
  {
    // save shader code to return it by 'GetShaderCode'
    shader_code_ = GetStandardValue(kShaderCode).value<QString>();

    // the code of the shader has changed.
    // Remove all inputs and re-parse the code
    // to fix shader name and input parameters.
    onShaderCodeChanged();
  }
}

QString ShaderFilterNode::Description() const
{
  return tr("a filter made by a GLSL shader code");
}

void ShaderFilterNode::Retranslate()
{
  // Retranslate the only fixed input.
  // Other inputs are read from the shader code
  SetInputName( kShaderCode, tr("Shader code"));
}

ShaderCode ShaderFilterNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)

  return ShaderCode(shader_code_);
}

void ShaderFilterNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job;

  job.InsertValue(value);
  job.InsertValue(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, globals.resolution(), this));

  // If there's no shader code, no need to run an operation
  if (shader_code_.trimmed() != QString()) {
    table->Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
  }
}

void ShaderFilterNode::parseShaderCode()
{
  ShaderInputsParser parser(shader_code_);

  parser.Parse();

  // update name, if defined in script
  SetLabel( parser.ShaderName());

  // update input list
  const QList< ShaderInputsParser::InputParam> & input_list = parser.InputList();
  QList< ShaderInputsParser::InputParam>::const_iterator it;

  for( it = input_list.begin(); it != input_list.end(); ++it) {

    if (HasInputWithID(it->uniform_name) == false) {
      AddInput( it->uniform_name, it->type, it->default_value, it->flags );
      input_list_.append( it->uniform_name);
    }

    SetInputName( it->uniform_name, it->readable_name);
    SetInputProperty( it->uniform_name, QStringLiteral("min"), it->min);
    SetInputProperty( it->uniform_name, QStringLiteral("max"), it->max);
  }
}

}  // namespace olive

