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

#include "generic.h"

#include <QRegularExpression>
#include <QFileInfo>

namespace olive {

const QString GenericFilterNode::kTextureInput = QStringLiteral("tex_in");


GenericFilterNode::GenericFilterNode(const QString & srcFilePath) :
  src_file_path_(srcFilePath)
{
  filter_name_ = QFileInfo(srcFilePath).baseName();

  // Fixed input: an image to work on. More textures can be added as inputs
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  // search source file for parameters to be added to the node panel.
  // Exposed parametrs are the ones that follow the convention described in
  // function 'searchParametersInSourceFile'
  //
  searchParametersInSourceFile( srcFilePath);
}

Node *GenericFilterNode::copy() const
{
  return new GenericFilterNode( src_file_path_);
}

QString GenericFilterNode::Name() const
{
  return QString("GLSL: %1").arg(filter_name_);
}

QString GenericFilterNode::id() const
{
  // give an ID that depends on file name, but avoid white spaces
  QString tag = filter_name_;
  tag.replace( QRegularExpression("\\s"), "_");
  return QStringLiteral("org.olivevideoeditor.Olive.glsl.%1").arg(tag);
}

QVector<Node::CategoryID> GenericFilterNode::Category() const
{
  return {kCategoryFilter};
}

QString GenericFilterNode::Description() const
{
  return tr("custom filter.");
}

void GenericFilterNode::Retranslate()
{
  // Retranslate the only fixed input.
  // Other inputs are read from the script file
  SetInputName( kTextureInput, tr("Input"));
}

ShaderCode GenericFilterNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)

  QString source = FileFunctions::ReadFileAsString( src_file_path_);

  return ShaderCode(source);
}

void GenericFilterNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job;

  job.InsertValue(value);
  job.InsertValue(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, globals.resolution(), this));

  // If there's no texture, no need to run an operation
  if (!job.GetValue(kTextureInput).data().isNull()) {
    table->Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);

  } else {
    // If we're not performing the generic job, just push the texture
    table->Push(job.GetValue(kTextureInput));
  }
}

// parse the GLSL file specified by the user and search for inputs to be exposed in node panel.
// Search, within the file, parameters that are exposed as inputs to the according to the following convention:
// - a script variable that starts with the following prefixes, is exposed as an input of type:
//      prefix      type
//-------------------------------
//      inColor     NodeValue::kColor
//      inTexture   NodeValue::kTexture
//      inFloat     NodeValue::kFloat
//      inInt       NodeValue::inInt
//      inBool      NodeValue::inBool
//
// After semicolon, a comment is expected with the name of the input that appares in GUI.
// Such name is required, otherwise the variable will not be exposed
//
void GenericFilterNode::searchParametersInSourceFile( const QString & src_file_path)
{
  static QRegularExpression param_regEx("uniform\\s+([A-Za-z0-9_]+)\\s+(in[A-Za-z0-9_]+)\\s*;\\s*//\\s*(.*)\\s*$");
  QRegularExpressionMatch match;

  QString source = FileFunctions::ReadFileAsString( src_file_path);

  QStringList lines = source.split(QRegularExpression("[\\n\\r]+"), Qt::SkipEmptyParts);

  for ( QString line : lines) {

    match = param_regEx.match( line);

    if (match.isValid()) {

      QString type = match.captured(1);
      QString param_name = match.captured(2);

      NodeValue::Type paramType = typeFromName( param_name);

      if (paramType != NodeValue::kNone) {
        AddInput( param_name, paramType);

        // use name written in comment after semicolon
        SetInputName( param_name, match.captured(3));
      }
    }
  }
}

NodeValue::Type GenericFilterNode::typeFromName(const QString &paramName)
{
  if (paramName.startsWith("inColor"))
    return NodeValue::kColor;

  if (paramName.startsWith("inTexture"))
    return NodeValue::kTexture;

  if (paramName.startsWith("inFloat"))
    return NodeValue::kFloat;

  if (paramName.startsWith("inInt"))
    return NodeValue::kInt;

  if (paramName.startsWith("inBool"))
    return NodeValue::kBoolean;

  return NodeValue::kNone;
}

}
