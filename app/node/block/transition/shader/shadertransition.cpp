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

#include "shadertransition.h"

#include <QOpenGLShader>

#include "node/filter/shader/shaderinputsparser.h"


namespace olive {

namespace {
const QString DEFAULT_NAME = "My Transition";
const QString DEFAULT_VERSION = "0.1";
const QString DEFAULT_DESCRIPTION = "a transition that's still to be implemented";
}

// a default shader transition
const QString olive::ShaderTransition::FRAG_TEMPLATE = QString(
"#version 150""\n"
"//OVE shader_name: %1""\n"
"//OVE shader_description: %2\n"
"//OVE shader_version: %3\n\n"
"//OVE end""\n"
"\n"
"uniform sampler2D out_block_in;""\n"
"uniform sampler2D in_block_in;""\n"
"uniform bool out_block_in_enabled;""\n"
"uniform bool in_block_in_enabled;""\n"
"\n"
"uniform int curve_in;""\n"
"\n"
"uniform float ove_tprog_all;""\n"
"\n"
"in vec2 ove_texcoord;""\n"
"out vec4 frag_color;""\n"
"""\n"
"void main(void) {""\n"
"    frag_color = texture(in_block_in, ove_texcoord*step( 1.0 - ove_tprog_all, ove_texcoord.xy)) +""\n"
"                 texture(out_block_in, ove_texcoord*step( ove_tprog_all, ove_texcoord.xy));""\n"
"}""\n").arg(DEFAULT_NAME, DEFAULT_DESCRIPTION, DEFAULT_VERSION);

const QString olive::ShaderTransition::VERTEX_TEMPLATE = QString(
    "#version 150\n"
    "// No metadata will be parsed from this shader\n"
    "// You can decalre 'uniform' inputs defined in fragment shader\n"
    "uniform mat4 ove_mvpmat;\n"
    "\n"
    "in vec4 a_position;\n"
    "in vec2 a_texcoord;\n"
    "\n"
    "out vec2 ove_texcoord;\n"
    "\n"
    "void main() {\n"
    "    gl_Position = ove_mvpmat * a_position;\n"
    "    ove_texcoord = a_texcoord;\n"
    "}\n");


#define super TransitionBlock

const QString ShaderTransition::kFragShaderCode = QStringLiteral("frag_source");
const QString ShaderTransition::kUseVertexCode = QStringLiteral("use_vertex");
const QString ShaderTransition::kVertexShaderCode = QStringLiteral("vertex_source");
const QString ShaderTransition::kOutputMessages = QStringLiteral("issues");

ShaderTransition::ShaderTransition() :
  invalidate_code_flag_(false),
  shader_name_(DEFAULT_NAME),
  shader_version_(DEFAULT_VERSION),
  shader_description_(DEFAULT_DESCRIPTION),
  number_of_iterations_(1),
  use_vertex_shader(false)
{
  // Option to use vertex shader. When false, the default vertex shader is used
  AddInput(kUseVertexCode, NodeValue::kBoolean, QVariant::fromValue<bool>(false),
           InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));
  AddInput(kVertexShaderCode, NodeValue::kText, QVariant::fromValue<QString>(VERTEX_TEMPLATE),
           InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable | kInputFlagHidden));
  // Full code of fragment shader. Inputs to be exposed are defined within the shader code
  // with mark-up comments.
  AddInput(kFragShaderCode, NodeValue::kText, QVariant::fromValue<QString>(FRAG_TEMPLATE),
           InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  // Output messages of shader parser
  AddInput(kOutputMessages, NodeValue::kText, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  // mark this text input as code, so it will be edited with code editor
  SetInputProperty( kFragShaderCode, QStringLiteral("text_type"), QString("shader_code"));
  SetInputProperty( kVertexShaderCode, QStringLiteral("text_type"), QString("shader_code"));
  // mark this text input as output messages
  SetInputProperty( kOutputMessages, QStringLiteral("text_type"), QString("shader_issues"));
}

QString ShaderTransition::Name() const
{
  return tr("GLSL Transition");
}

QString ShaderTransition::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.transitionshader");
}

QVector<Node::CategoryID> ShaderTransition::Category() const
{
  return {kCategoryTransition};
}

void ShaderTransition::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(element)

  if ((input == kFragShaderCode) || (input == kVertexShaderCode)) {
    // for some reason, this function is called more than once for each input
    // of each instance. Parse code only if it has changed
    QString new_frag_code = GetStandardValue(kFragShaderCode).value<QString>();
    QString new_vertex_code = GetStandardValue(kVertexShaderCode).value<QString>();

    if ((frag_shader_code_ != new_frag_code) ||
        (vertex_shader_code_ != new_vertex_code)) {

      frag_shader_code_ = new_frag_code;
      vertex_shader_code_ = new_vertex_code;
      output_messages_.clear();

      // the code of the shader has changed.
      // Remove all inputs and re-parse the code
      // to fix shader name and input parameters.
      parseShaderMetadata();

      // check for syntax
      checkShaderSyntax();

      SetStandardValue( kOutputMessages, output_messages_.isEmpty() ? tr("None") : output_messages_);

      invalidate_code_flag_ = true;
    }
  } else if (input == kUseVertexCode) {
    bool use_vertex_code = GetStandardValue(kUseVertexCode).toBool();
    // hide or un-hide the widget for vertex shader
    SetInputFlag( kVertexShaderCode, kInputFlagHidden, ! use_vertex_code);

    // recalculate shader
    invalidate_code_flag_ = true;
  }
}

bool ShaderTransition::ShaderCodeInvalidateFlag() const
{
  bool invalid = invalidate_code_flag_;
  invalidate_code_flag_ = false;
  return invalid;
}

void ShaderTransition::getMetadata(QString &name, QString &description, QString &version) const
{
  name = shader_name_;
  description = shader_description_;
  version = shader_version_;
}

QString ShaderTransition::Description() const
{
  return tr("a transition made by a GLSL shader code");
}

void ShaderTransition::Retranslate()
{
  super::Retranslate();

  // Retranslate only fixed inputs.
  // Other inputs are read from the shader code
  SetInputName( kUseVertexCode, tr("Use vertex shader"));
  SetInputName( kVertexShaderCode, tr("Vertex Shader"));
  SetInputName( kFragShaderCode, tr("Fragment Shader"));
  SetInputName( kOutputMessages, tr("Issues"));
}

ShaderCode ShaderTransition::GetShaderCode(const Node::ShaderRequest &request) const
{
  Q_UNUSED(request);
  const QString & vertex_code = (GetStandardValue(kUseVertexCode).toBool()) ? vertex_shader_code_ : QString();

  return ShaderCode(frag_shader_code_, vertex_code);
}


void ShaderTransition::ShaderJobEvent(const NodeValueRow &value, ShaderJob *job) const
{
  // add user Inputs
  for( const QString & name : user_input_list_)
  {
    job->Insert( name, value);
  }

  // resolution
  TexturePtr p = value[QStringLiteral("in_block_in")].toTexture();

  if (p) {
    job->Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, p->virtual_resolution(), this));
  }

  job->SetShaderID(GetLabel());  // TBD: label may not be enough to identify a shader univocally

  if (number_of_iterations_ != 1) {
    job->SetIterations( number_of_iterations_, kInBlockInput);
  }
}


void ShaderTransition::checkShaderSyntax()
{
  bool ok;

  QOpenGLShader frag_shader(QOpenGLShader::Fragment);
  ok = compileShaderCode( frag_shader, frag_shader_code_);

  if (ok == false)
  {
    output_messages_ += QString("Fragment Shader compilation errors:\n") + frag_shader.log();
  }

  bool use_vertex_code = GetStandardValue(kUseVertexCode).toBool();

  if (use_vertex_code) {
    QOpenGLShader vertex_shader(QOpenGLShader::Vertex);
    ok = compileShaderCode( vertex_shader, vertex_shader_code_);

    if (ok == false)
    {
      output_messages_ += QString("Vertex Shader compilation errors:\n") + vertex_shader.log();
    }
  }
}

bool ShaderTransition::compileShaderCode( QOpenGLShader & shader, const QString & souce_code)
{
  bool ok;

  if (souce_code.startsWith("#version")) {

    ok=shader.compileSourceCode( souce_code);
  } else {

    // NOTE: this replicates the preamble added in compilation (see openglrenderer.cpp for reference).
    // This must be aligned if preamble changes
    ok=shader.compileSourceCode( "#version 150\n\n"
                                 "precision highp float;\n\n" + souce_code);
  }

  return ok;
}


void ShaderTransition::parseShaderMetadata()
{
  // only fragment shader has metadata
  ShaderInputsParser parser(frag_shader_code_);

  parser.Parse();

  reportErrorList( parser);
  updateInputList( parser);
  updateGizmoList();

  // update name, if defined in script; otherwise use a default.
  QString label = (parser.ShaderName().isEmpty()) ? "unnamed" : parser.ShaderName();
  SetLabel( label);
  shader_name_ = label;
  shader_description_ = parser.ShaderDescription();
  shader_version_ = parser.ShaderVersion();
  number_of_iterations_ = parser.NumberOfIterations();

  metadataChanged( shader_name_, shader_description_, shader_version_);
}

void ShaderTransition::reportErrorList( const ShaderInputsParser & parser)
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

void ShaderTransition::updateInputList( const ShaderInputsParser & parser)
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


  // compare 'new_input_list' and 'user_input_list_' to find deleted inputs.
  checkDeletedInputs( new_input_list);

  // update inputs
  user_input_list_.clear();
  user_input_list_ = new_input_list;

  emit InputListChanged();
}

// this function assumes that 'updateInputList' has been called already.
// Add a point gizmo for each 'kVec2' that does not already have one
void ShaderTransition::updateGizmoList()
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
          g->SetVisible( true);


          handle_table_.insert( aInput, g);
        }

        PointGizmo * g = handle_table_[aInput];
        g->SetShape( handle_shape_table_.value( aInput, PointGizmo::kSquare));
        g->setProperty("color", QVariant::fromValue<QColor>(handle_color_table_.value(aInput, Qt::white)));
      }
    }
  }
}

void ShaderTransition::checkDeletedInputs(const QStringList & new_inputs)
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

void ShaderTransition::GizmoDragMove(double x, double y, const Qt::KeyboardModifiers & /*modifiers*/)
{
  DraggableGizmo *gizmo = static_cast<DraggableGizmo*>(sender());

  Q_ASSERT( resolution_.x() != 0.);
  Q_ASSERT( resolution_.y() != 0.);

  gizmo->GetDraggers()[0].Drag( x/resolution_.x());
  gizmo->GetDraggers()[1].Drag( y/resolution_.y());
}

void ShaderTransition::UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals & globals)
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

}  // olive

