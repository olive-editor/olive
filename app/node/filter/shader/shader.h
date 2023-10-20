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

#ifndef SHADERFILTERNODE_H
#define SHADERFILTERNODE_H

#include "node/node.h"
#include "node/gizmo/point.h"

class QOpenGLShader;


namespace olive {

class ShaderInputsParser;


/** @brief
 * A node that implements a filter with a GLSL script. The inputs of this node
 * are defined in GLSL by markup comments
 */
class ShaderFilterNode : public Node
{
  Q_OBJECT
public:
  ShaderFilterNode();

  NODE_DEFAULT_DESTRUCTOR(ShaderFilterNode)

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual void UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals &globals) override;

  virtual ShaderCode GetShaderCode(const ShaderRequest &request) const override;
  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;
  void InputValueChangedEvent(const QString &input, int element) override;

  bool ShaderCodeInvalidateFlag() const override;

  void getMetadata( QString & name, QString & description, QString & version) const;

  static const QString kTextureInput;
  static const QString kFragShaderCode;
  static const QString kUseVertexCode;
  static const QString kVertexShaderCode;
  static const QString kOutputMessages;

signals:
  void metadataChanged( const QString & name, const QString & description, const QString & version);

private:
  void parseShaderMetadata();
  void checkShaderSyntax();
  bool compileShaderCode( QOpenGLShader &shader, const QString &souce_code);
  void reportErrorList( const ShaderInputsParser & parser);
  void updateInputList( const ShaderInputsParser & parser);
  void updateGizmoList();
  void checkDeletedInputs( const QStringList & new_inputs);


protected slots:
  virtual void GizmoDragMove(double x, double y, const Qt::KeyboardModifiers &modifiers) override;

private:
  // set to true when shader code changes. Must be mutable because
  // it is reset when it is read
  mutable bool invalidate_code_flag_;

  // source GLSL code
  QString frag_shader_code_;
  QString vertex_shader_code_;
  // name of filter declared in metadata
  QString shader_name_;
  // version decalred in metadata
  QString shader_version_;
  // description in metadata
  QString shader_description_;
  // error in metadata or shader
  QString output_messages_;
  // name of default texture input
  QString main_input_name_;
  // number of times fragment shader is invoked
  int number_of_iterations_;
  // user defined inputs
  QStringList user_input_list_;
  // input of type vec2 to gizmo map
  QMap<QString, PointGizmo *> handle_table_;
  // shape for points in 'handle_table_'
  QMap<QString, PointGizmo::Shape> handle_shape_table_;
  // color for points in 'handle_table_'
  QMap<QString, QColor> handle_color_table_;
  // when false, a default vertex shader is used
  bool use_vertex_shader;

  QVector2D resolution_;

  static const QString FRAG_TEMPLATE;
  static const QString VERTEX_TEMPLATE;
};

}

#endif // SHADERFILTERNODE_H
