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
#include <QMessageBox>

#include <QDialog>
#include <QLabel>
#include <QLayout>

#include "shaderinputsparser.h"

namespace olive {

const QString ShaderFilterNode::kShaderCode = QStringLiteral("source");
const QString ShaderFilterNode::kOutputMessages = QStringLiteral("issues");


ShaderFilterNode::ShaderFilterNode()
{
  // Full code of the shader. Inputs to be exposed are defined within the shader code
  // with mark-up comments.
  AddInput(kShaderCode, NodeValue::kText, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  // Output messages of shader parser
  AddInput(kOutputMessages, NodeValue::kText, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  // mark this text input as code, so it will be edited with code editor
  SetInputProperty( kShaderCode, QStringLiteral("text_type"), QString("shader_code"));
  // mark this text input as output messages
  SetInputProperty( kOutputMessages, QStringLiteral("text_type"), QString("shader_issues"));
}

Node *ShaderFilterNode::copy() const
{
  ShaderFilterNode * new_node = new ShaderFilterNode();

  // copy all inputs not created in constructor
  CopyInputs( this, new_node, false);

  return new_node;
}

QString ShaderFilterNode::Name() const
{
  return QString("Shader");
}

QString ShaderFilterNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.shader");
}

QVector<Node::CategoryID> ShaderFilterNode::Category() const
{
  return {kCategoryFilter};
}

void ShaderFilterNode::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(element)


  if (input == kShaderCode) {
    // for some reason, this function is called more than once for each input
    // of each instance. Parse code only if it has changed
    QString new_code = GetStandardValue(kShaderCode).value<QString>();

    if (shader_code_ != new_code) {

      shader_code_ = new_code;

      // the code of the shader has changed.
      // Remove all inputs and re-parse the code
      // to fix shader name and input parameters.
      onShaderCodeChanged();
    }
  }
}

void olive::ShaderFilterNode::onShaderCodeChanged()
{
  // pre-remove all inputs ...
  for (QString oldInput : user_input_list_)
  {
    if (HasInputWithID(oldInput)) {
      RemoveInput(oldInput);
    }
  }
  user_input_list_.clear();

  // ... and create new inputs
  parseShaderCode();

  qDebug() << "parsed shader code for " << GetLabel() << " @ " << (uint64_t)this;
}

QString ShaderFilterNode::Description() const
{
  return tr("a filter made by a GLSL shader code");
}

void ShaderFilterNode::Retranslate()
{
  // Retranslate the only fixed inputs.
  // Other inputs are read from the shader code
  SetInputName( kShaderCode, tr("Shader code"));
  SetInputName( kOutputMessages, tr("Issues"));
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

  reportErrorList( parser);
  updateInputList( parser);

  // update name, if defined in script; otherwise use a default.
  QString label = (parser.ShaderName().isEmpty()) ? "unnamed" : parser.ShaderName();
  SetLabel( label);
}

void ShaderFilterNode::reportErrorList( const ShaderInputsParser & parser)
{
  const QList<ShaderInputsParser::Error> & errors = parser.ErrorList();

  QString message = QString(tr("There are %1 issues.\n").arg(errors.size()));

  for (ShaderInputsParser::Error e : errors ) {
    message.append(QString("\"%1\" line %2: %3\n").
                   arg( parser.ShaderName()).arg(e.line).arg(e.issue));
  }

  SetStandardValue( kOutputMessages, QVariant::fromValue<QString>(message));
}

void ShaderFilterNode::updateInputList( const ShaderInputsParser & parser)
{
  const QList< ShaderInputsParser::InputParam> & input_list = parser.InputList();
  QList< ShaderInputsParser::InputParam>::const_iterator it;

  QStringList new_input_list;

  for( it = input_list.begin(); it != input_list.end(); ++it) {

    if (HasInputWithID(it->uniform_name) == false) {
      AddInput( it->uniform_name, it->type, it->default_value, it->flags );
      new_input_list.append( it->uniform_name);
    }

    SetInputName( it->uniform_name, it->human_name);
    SetInputProperty( it->uniform_name, QStringLiteral("min"), it->min);
    SetInputProperty( it->uniform_name, QStringLiteral("max"), it->max);
  }

  // compare 'new_input_list' and 'user_input_list_' to find deleted inputs.
  checkDeletedInputs( new_input_list);

  // update inputs
  user_input_list_.clear();
  user_input_list_ = new_input_list;

  emit InputListChanged();
}

void ShaderFilterNode::checkDeletedInputs(const QStringList & new_inputs)
{
  // search old inputs that are not present in new inputs
  for( const QString & input : user_input_list_) {
    if (new_inputs.contains(input) == false) {
      InputRemoved( input);
    }
  }
}

}  // namespace olive

