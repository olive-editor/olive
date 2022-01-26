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

#ifndef CORNERPINDISTORTNODE_H
#define CORNERPINDISTORTNODE_H

#include <QVector2D>

#include "node/inputdragger.h"
#include "node/node.h"

namespace olive {
class CornerPinDistortNode : public Node
{
  Q_OBJECT
public:
  CornerPinDistortNode();

  NODE_DEFAULT_DESTRUCTOR(CornerPinDistortNode)

  virtual Node* copy() const override
  {
    return new CornerPinDistortNode();
  }

  virtual QString Name() const override
  {
    return tr("Corner Pin");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.cornerpin");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryDistort};
  }

  virtual QString Description() const override
  {
    return tr("Distort the image by dragging the corners around");
  }

  virtual void Retranslate() override;

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  virtual ShaderCode GetShaderCode(const QString &shader_id) const override;

  virtual bool HasGizmos() const override
  {
    return true;
  }

  virtual void DrawGizmos(const NodeValueRow& row, const NodeGlobals &globals, QPainter *p) override;

  virtual bool GizmoPress(const NodeValueRow& row, const NodeGlobals &globals, const QPointF &p) override;
  virtual void GizmoMove(const QPointF &p, const rational &time, const Qt::KeyboardModifiers &modifiers) override;
  virtual void GizmoRelease(MultiUndoCommand *command) override;

  /**
   * @brief Convenience function - converts the 2D slider values from being
   * an offset to the actual pixel value.
   */
  QPointF ValueToPixel(int value, const NodeValueRow &row, const QVector2D &resolution) const;

  static const QString kTextureInput;
  static const QString kPerspectiveInput;
  static const QString kTopLeftInput;
  static const QString kTopRightInput;
  static const QString kBottomRightInput;
  static const QString kBottomLeftInput;

private:
  // Gizmo variables
  static const int kGizmoCornerCount = 4;
  QRectF gizmo_resize_handle_[kGizmoCornerCount];
  QRectF gizmo_whole_rect_;

  int gizmo_drag_;
  QVector<NodeInputDragger> gizmo_dragger_;
  QVariant gizmo_start_;
  QPointF gizmo_drag_start_;
  QVector2D gizmo_res_;

};

}


#endif // CORNERPINDISTORTNODE_H
