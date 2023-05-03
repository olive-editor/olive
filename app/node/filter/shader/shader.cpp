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

#include "shader.h"

#include <QRegularExpression>
#include <QFileInfo>
#include <QMessageBox>

#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QOpenGLShader>
#include <QRandomGenerator>

#include "shaderinputsparser.h"


namespace olive {

#define super Node


namespace  {
// a default shader that replicates to output a texture input.
const QString TEMPLATE(
    "//OVE shader_name: \n"
    "//OVE shader_description: \n\n"
    "//OVE main_input_name: Input\n"
    "uniform sampler2D tex_in;\n\n"
    "//OVE end\n\n\n"
    "// pixel coordinates in range [0..1]x[0..1]\n"
    "in vec2 ove_texcoord;\n"
    "// output color\n"
    "out vec4 frag_color;\n\n"
    "void main(void) {\n"
    "   vec4 textureColor = texture2D(tex_in, ove_texcoord);\n"
    "   frag_color= textureColor.yzxt;\n"
    "}\n");

}  // namespace

const QString ShaderFilterNode::kTextureInput = QStringLiteral("tex_in");
const QString ShaderFilterNode::kShaderCode = QStringLiteral("source");
const QString ShaderFilterNode::kOutputMessages = QStringLiteral("issues");


ShaderFilterNode::ShaderFilterNode():
  invalidate_code_flag_(false),
  main_input_name_("Input")
{
  // Full code of the shader. Inputs to be exposed are defined within the shader code
  // with mark-up comments.
  AddInput(kShaderCode, NodeValue::kText, QVariant::fromValue<QString>(TEMPLATE),
           InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  // Output messages of shader parser
  AddInput(kOutputMessages, NodeValue::kText, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  // mark this text input as code, so it will be edited with code editor
  SetInputProperty( kShaderCode, QStringLiteral("text_type"), QString("shader_code"));
  // mark this text input as output messages
  SetInputProperty( kOutputMessages, QStringLiteral("text_type"), QString("shader_issues"));

  // A default texture. Must be connected even for shaders that do not require a texture.
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));
  SetEffectInput(kTextureInput);

  SetFlag(kVideoEffect);
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
      output_messages_.clear();

      // the code of the shader has changed.
      // Remove all inputs and re-parse the code
      // to fix shader name and input parameters.
      parseShaderCode();

      // check for syntax
      checkShaderSyntax();

      SetStandardValue( kOutputMessages, output_messages_.isEmpty() ? tr("None") : output_messages_);

      invalidate_code_flag_ = true;
    }
  }
}

bool ShaderFilterNode::ShaderCodeInvalidateFlag() const
{
  bool invalid = invalidate_code_flag_;
  invalidate_code_flag_ = false;
  return invalid;
}


QString ShaderFilterNode::Description() const
{
  return tr("a filter made by a GLSL shader code");
}

void ShaderFilterNode::Retranslate()
{
  super::Retranslate();

  // Retranslate only fixed inputs.
  // Other inputs are read from the shader code
  SetInputName( kShaderCode, tr("Shader code"));
  SetInputName( kOutputMessages, tr("Issues"));
}

ShaderCode ShaderFilterNode::GetShaderCode(const Node::ShaderRequest &request) const
{
  Q_UNUSED(request);
  return ShaderCode(shader_code_);
}


void ShaderFilterNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  if (TexturePtr tex = value[kTextureInput].toTexture()) {
      ShaderJob job(value);
      job.SetShaderID(GetLabel());  // TBD: label may not be enough to identify a shader univocally
      job.Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, tex->virtual_resolution(), this));
      table->Push(NodeValue::kTexture, tex->toJob(job), this);
  }
}



void olive::ShaderFilterNode::checkShaderSyntax()
{
  QOpenGLShader shader(QOpenGLShader::Fragment);
  bool ok;

  if (shader_code_.startsWith("#version")) {

    ok=shader.compileSourceCode( shader_code_);
  } else {

    // NOTE: this replicates the preamble added in compilation (see openglrenderer.cpp for reference).
    // This must be aligned if preamble changes
    ok=shader.compileSourceCode( "#version 150\n\n"
                                 "\n\n" + shader_code_);
  }

  if (ok == false)
  {
    output_messages_ += QString("Compilation errors:\n") + shader.log();
  }
}


void ShaderFilterNode::parseShaderCode()
{
  ShaderInputsParser parser(shader_code_);

  parser.Parse();

  reportErrorList( parser);
  updateInputList( parser);
  updateGizmoList();

  // update name, if defined in script; otherwise use a default.
  QString label = (parser.ShaderName().isEmpty()) ? "unnamed" : parser.ShaderName();
  SetLabel( label);
}

void ShaderFilterNode::reportErrorList( const ShaderInputsParser & parser)
{
  const QList<ShaderInputsParser::Error> & errors = parser.ErrorList();

  QString message;
  if (errors.size() > 0) {
    message = QString(tr("There are %1 metadata issues.\n").arg(errors.size()));
  }

  for (const ShaderInputsParser::Error & e : errors ) {
    message.append(QString("\"%1\" line %2: %3\n").
                   arg( parser.ShaderName()).arg(e.line).arg(e.issue));
  }

  output_messages_ += message;
}

void ShaderFilterNode::updateInputList( const ShaderInputsParser & parser)
{
  const QList< ShaderInputsParser::InputParam> & input_list = parser.InputList();
  QList< ShaderInputsParser::InputParam>::const_iterator it;

  QStringList new_input_list;

  for( it = input_list.begin(); it != input_list.end(); ++it) {

    new_input_list.append( it->uniform_name);

    if (HasInputWithID(it->uniform_name) == false) {
      // this is a new input
      AddInput( it->uniform_name, it->type, it->default_value, it->flags );

    }
    else {
      // input already present. Check if some feature has changed
      if (GetInputDataType( it->uniform_name) != it->type) {
        SetInputDataType( it->uniform_name, it->type);
      }

      if (GetInputFlags( it->uniform_name).value() != it->flags.value()) {

        SetInputFlag( it->uniform_name, kInputFlagArray, (it->flags & kInputFlagArray) ? true : false);
        SetInputFlag(it->uniform_name, kInputFlagNotKeyframable, (it->flags & kInputFlagNotKeyframable) ? true : false);
        SetInputFlag( it->uniform_name, kInputFlagNotConnectable, (it->flags & kInputFlagNotConnectable) ? true : false);
        SetInputFlag( it->uniform_name, kInputFlagHidden, (it->flags & kInputFlagHidden) ? true : false);
        SetInputFlag( it->uniform_name, kInputFlagIgnoreInvalidations, (it->flags & kInputFlagIgnoreInvalidations) ? true : false);
      }
    }

    // set other features even if not changed
    SetInputName( it->uniform_name, it->human_name);
    if (it->min.isValid()) {
      SetInputProperty( it->uniform_name, QStringLiteral("min"), it->min);
    }
    if (it->max.isValid()) {
      SetInputProperty( it->uniform_name, QStringLiteral("max"), it->max);
    }

    if (it->type == NodeValue::kCombo) {
      SetComboBoxStrings(it->uniform_name, it->values);
    }

    if (it->type == NodeValue::kVec2) {
      handle_shape_table_.insert( it->uniform_name, it->pointShape);
      handle_color_table_.insert( it->uniform_name, it->gizmoColor);
    }
  }

  // main input
  main_input_name_ = (parser.MainInputName().isEmpty()) ? "Input" : parser.MainInputName();
  SetInputName( kTextureInput, main_input_name_);

  // compare 'new_input_list' and 'user_input_list_' to find deleted inputs.
  checkDeletedInputs( new_input_list);

  // update inputs
  user_input_list_.clear();
  user_input_list_ = new_input_list;

  emit InputListChanged();
}

// this function assumes that 'updateInputList' has been called already.
// Add a point gizmo for each 'kVec2' that does not already have one
void ShaderFilterNode::updateGizmoList()
{
  for (QString & aInput : user_input_list_)
  {
    if (HasInputWithID(aInput)) {
      if (GetInputDataType(aInput) == NodeValue::kVec2) {

        // is point new?
        if (handle_table_.contains(aInput) == false) {
          PointGizmo * g = AddDraggableGizmo<PointGizmo>();
          g->AddInput(NodeKeyframeTrackReference(NodeInput(this, aInput), 0));
          g->AddInput(NodeKeyframeTrackReference(NodeInput(this, aInput), 1));
          g->SetDragValueBehavior(PointGizmo::kAbsolute);

          handle_table_.insert( aInput, g);
        }

        PointGizmo * g = handle_table_[aInput];
        g->SetShape( handle_shape_table_.value( aInput, PointGizmo::kSquare));
        g->setProperty("color", QVariant::fromValue<QColor>(handle_color_table_.value(aInput, Qt::white)));
      }
    }
  }
}

void ShaderFilterNode::checkDeletedInputs(const QStringList & new_inputs)
{
  // search old inputs that are not present in new inputs
  for( QString & input : user_input_list_) {
    if (new_inputs.contains(input) == false) {
      RemoveInput( input);

      // remove gizmo, if any
      if (handle_table_.contains(input)) {
        RemoveGizmo( handle_table_[input]);
        handle_table_.remove( input);
        handle_shape_table_.remove( input);
        handle_color_table_.remove( input);
      }
    }
  }
}

void ShaderFilterNode::GizmoDragMove(double x, double y, const Qt::KeyboardModifiers & /*modifiers*/)
{
  DraggableGizmo *gizmo = static_cast<DraggableGizmo*>(sender());

  Q_ASSERT( resolution_.x() != 0.);
  Q_ASSERT( resolution_.y() != 0.);

  gizmo->GetDraggers()[0].Drag( x/resolution_.x());
  gizmo->GetDraggers()[1].Drag( y/resolution_.y());
}

void ShaderFilterNode::UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals & globals)
{
  resolution_  = globals.vparams().resolution();

  for (QString & aInput : user_input_list_)
  {
    if (HasInputWithID(aInput)) {
      if (row[aInput].type() == NodeValue::kVec2) {
        QVector2D pos_vec = row[aInput].value<QVector2D>() * resolution_;
        QPointF pos( pos_vec.x(), pos_vec.y());

        handle_table_[aInput]->SetPoint( pos);
      }
    }
  }
}


}  // namespace olive

