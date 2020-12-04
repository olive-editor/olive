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

#ifndef TRANSFORMDISTORTNODE_H
#define TRANSFORMDISTORTNODE_H

#include "node/generator/matrix/matrix.h"

namespace olive {

class TransformDistortNode : public MatrixGenerator
{
  Q_OBJECT
public:
  TransformDistortNode();

  virtual Node* copy() const override
  {
    return new TransformDistortNode();
  }

  virtual QString Name() const override
  {
    return tr("Transform");
  }

  virtual QString ShortName() const override
  {
    return Node::ShortName();
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.transform");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryDistort};
  }

  virtual QString Description() const override
  {
    return tr("Transform an image in 2D space. Equivalent to multiplying by an orthographic matrix.");
  }

  virtual void Retranslate() override;

  virtual NodeValueTable Value(NodeValueDatabase& value) const override;

  virtual ShaderCode GetShaderCode(const QString& shader_id) const override;

  virtual bool HasGizmos() const override
  {
    return true;
  }

  virtual void DrawGizmos(NodeValueDatabase& db, QPainter *p) override;

  virtual bool GizmoPress(NodeValueDatabase& db, const QPointF &p) override;
  virtual void GizmoMove(const QPointF &p, const rational &time) override;
  virtual void GizmoRelease() override;

  NodeInput* texture_input() const
  {
    return texture_input_;
  }

  enum AutoScaleType {
    kAutoScaleNone,
    kAutoScaleFit,
    kAutoScaleFill,
    kAutoScaleStretch
  };

  static QMatrix4x4 AdjustMatrixByResolutions(const QMatrix4x4& mat,
                                              const QVector2D& sequence_res,
                                              const QVector2D& texture_res,
                                              AutoScaleType autoscale_type = kAutoScaleNone);

private:
  static QPointF CreateScalePoint(double x, double y, const QPointF& half_res, const QMatrix4x4& mat);

  NodeInput* texture_input_;
  NodeInput* autoscale_input_;
  NodeInput* interpolation_input_;

  // Gizmo variables
  NodeInput* gizmo_drag_;
  QVector<QVariant> gizmo_start_;
  QVector<NodeInputDragger> gizmo_dragger_;
  QPointF gizmo_drag_pos_;
  double gizmo_start_angle_;
  bool gizmo_scale_uniform_;

  enum GizmoScaleType {
    kGizmoScaleXOnly,
    kGizmoScaleYOnly,
    kGizmoScaleBoth
  };

  GizmoScaleType gizmo_scale_axes_;
  QMatrix4x4 gizmo_matrix_;
  QVector2D gizmo_scale_anchor_;

  // Gizmo on screen object storage
  QPolygonF gizmo_rect_;
  QRectF gizmo_anchor_pt_;
  QRectF gizmo_resize_handle_[kGizmoScaleCount];

};

}

#endif // TRANSFORMDISTORTNODE_H
