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

#ifndef ShaderFilterNode_H
#define ShaderFilterNode_H

#include "node/node.h"
#include "node/inputdragger.h"

namespace olive {

class ShaderInputsParser;
class MessageDialog;


/** @brief
 * A node that implements a GLSL script. The inputs of this node
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

  virtual ShaderCode GetShaderCode(const QString &shader_id) const override;
  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;
  void InputValueChangedEvent(const QString &input, int element) override;

  static const QString kShaderCode;

  static const QString kOutputMessages;

private:
  void parseShaderCode();
  void onShaderCodeChanged();
  void reportErrorList( const ShaderInputsParser & parser);
  void updateInputList( const ShaderInputsParser & parser);
  void checkDeletedInputs( const QStringList & new_inputs);

private:

  QString shader_code_;
  // user defined inputs
  QStringList user_input_list_;

  // two draggers for x and y, shared by all vec2 inputs
  NodeInputDragger dragger_[2];
  // one handle for every vec2 input
  QMap<QString, QRectF> handle_table_;

  QString currently_dragged_input_;
  QVector2D resolution_;

  // Node interface
public:
  bool HasGizmos() const override;
  void DrawGizmos(const NodeValueRow &row, const NodeGlobals &globals, QPainter *p) override;
  bool GizmoPress(const NodeValueRow &row, const NodeGlobals &globals, const QPointF &p) override;
  void GizmoMove(const QPointF &p, const rational &time, const Qt::KeyboardModifiers &modifiers) override;
  void GizmoRelease(MultiUndoCommand *command) override;
};

}

#endif // ShaderFilterNode_H
