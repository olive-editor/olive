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

#include <QOpenGLShader>

#include "shaderinputsparser.h"


namespace olive {

#define super Node

#define DEFAULT_FILTER_NAME "My Filter"


// a default shader filter
const QString ShaderFilterNode::FRAG_TEMPLATE = QString(
    "#version 150""\n"
    "//OVE shader_name: %1\n"
    "//OVE shader_description: a filter that's still to be implemented\n"
    "//OVE shader_version: 0.1\n\n"
    "//OVE main_input_name: Input\n"
    "uniform sampler2D tex_in;\n\n"
    "//OVE end\n\n\n"
    "// pixel coordinates in range [0..1]x[0..1]\n"
    "in vec2 ove_texcoord;\n"
    "// output color\n"
    "out vec4 frag_color;\n\n"
    "void main(void) {\n"
    "   vec4 textureColor = texture(tex_in, ove_texcoord);\n"
    "   frag_color= textureColor.brga;\n"
    "}\n").arg(DEFAULT_FILTER_NAME);

const QString ShaderFilterNode::VERTEX_TEMPLATE = QString(
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

const QString ShaderFilterNode::kTextureInput = QStringLiteral("tex_in");
const QString ShaderFilterNode::kFragShaderCode = QStringLiteral("frag_source");
const QString ShaderFilterNode::kUseVertexCode = QStringLiteral("use_vertex");
const QString ShaderFilterNode::kVertexShaderCode = QStringLiteral("vertex_source");
const QString ShaderFilterNode::kOutputMessages = QStringLiteral("issues");


ShaderFilterNode::ShaderFilterNode():
  invalidate_code_flag_(false),
  main_input_name_("Input"),
  number_of_iterations_(1),
  use_vertex_shader(false)
{
  // Option to use vertex shader. When false, the default vertex shader is used
  AddInput(kUseVertexCode, NodeValue::kBoolean, QVariant::fromValue<bool>(false),
           InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));
  AddInput(kVertexShaderCode, NodeValue::kText, QVariant::fromValue<QString>(VERTEX_TEMPLATE),
           InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));
  // Full code of fragment shader. Inputs to be exposed are defined within the shader code
  // with mark-up comments.
  AddInput(kFragShaderCode, NodeValue::kText, QVariant::fromValue<QString>(FRAG_TEMPLATE),
           InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  // Output messages of shader parser
  AddInput(kOutputMessages, NodeValue::kText, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  // mark this text input as code, so it will be edited with code editor
  SetInputProperty( kFragShaderCode, QStringLiteral("text_type"), QString("shader_code_frag"));
  SetInputProperty( kVertexShaderCode, QStringLiteral("text_type"), QString("shader_code_vert"));
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

bool ShaderFilterNode::ShaderCodeInvalidateFlag() const
{
  bool invalid = invalidate_code_flag_;
  invalidate_code_flag_ = false;
  return invalid;
}

void ShaderFilterNode::getMetadata(QString &name, QString &description, QString &version) const
{
  name = shader_name_;
  description = shader_description_;
  version = shader_version_;
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
  SetInputName( kUseVertexCode, tr("Use vertex shader"));
  SetInputName( kVertexShaderCode, tr("Vertex Shader"));
  SetInputName( kFragShaderCode, tr("Fragment Shader"));
  SetInputName( kOutputMessages, tr("Issues"));
}

ShaderCode ShaderFilterNode::GetShaderCode(const Node::ShaderRequest &request) const
{
  Q_UNUSED(request);
  const QString & vertex_code = (GetStandardValue(kUseVertexCode).toBool()) ? vertex_shader_code_ : QString();

  return ShaderCode(frag_shader_code_, vertex_code);
}


void ShaderFilterNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  if (TexturePtr tex = value[kTextureInput].toTexture()) {
      ShaderJob job(value);
      job.SetShaderID(GetLabel());  // TBD: label may not be enough to identify a shader univocally
      job.Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, tex->virtual_resolution(), this));

      if (number_of_iterations_ != 1) {
        job.SetIterations( number_of_iterations_, main_input_name_);
      }

      table->Push(NodeValue::kTexture, tex->toJob(job), this);
  }
}


void ShaderFilterNode::checkShaderSyntax()
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

bool ShaderFilterNode::compileShaderCode( QOpenGLShader & shader, const QString & souce_code)
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

void ShaderFilterNode::parseShaderMetadata()
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

