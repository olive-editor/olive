/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef CROPDISTORTNODE_H
#define CROPDISTORTNODE_H

#include <QVector2D>

#include "node/inputdragger.h"
#include "node/node.h"

namespace olive {

class CropDistortNode : public Node
{
public:
  CropDistortNode();

  virtual Node* copy() const override
  {
    return new CropDistortNode();
  }

  virtual QString Name() const override
  {
    return tr("Crop");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.crop");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryDistort};
  }

  virtual QString Description() const override
  {
    return tr("Crop the edges of an image.");
  }

  virtual void Retranslate() override;

  virtual NodeValueTable Value(const QString& output, NodeValueDatabase& value) const override;

  virtual ShaderCode GetShaderCode(const QString &shader_id) const override;

  virtual bool HasGizmos() const override
  {
    return true;
  }

  virtual void DrawGizmos(NodeValueDatabase& db, QPainter *p) override;

  virtual bool GizmoPress(NodeValueDatabase& db, const QPointF &p) override;
  virtual void GizmoMove(const QPointF &p, const rational &time) override;
  virtual void GizmoRelease() override;

  static const QString kTextureInput;
  static const QString kLeftInput;
  static const QString kTopInput;
  static const QString kRightInput;
  static const QString kBottomInput;
  static const QString kFeatherInput;

private:
  void CreateCropSideInput(const QString& id);

  // Gizmo variables
  QRectF gizmo_resize_handle_[kGizmoScaleCount];
  QRectF gizmo_whole_rect_;

  enum GizmoDragDirection {
    kGizmoNone = 0x0,
    kGizmoLeft = 0x1,
    kGizmoTop = 0x2,
    kGizmoRight = 0x4,
    kGizmoBottom = 0x8,
    kGizmoRectangle = 0xFF
  };

  int gizmo_drag_;
  QVector<NodeInputDragger> gizmo_dragger_;
  QVector<QVariant> gizmo_start_;
  QPointF gizmo_drag_start_;
  QVector2D gizmo_res_;

};

}

#endif // CROPDISTORTNODE_H
