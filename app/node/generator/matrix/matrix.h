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

#ifndef MATRIXGENERATOR_H
#define MATRIXGENERATOR_H

#include <QVector2D>

#include "node/node.h"
#include "node/inputdragger.h"

OLIVE_NAMESPACE_ENTER

class MatrixGenerator : public Node
{
  Q_OBJECT
public:
  MatrixGenerator();

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString ShortName() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual NodeValueTable Value(NodeValueDatabase& value) const override;

  virtual bool HasGizmos() const override;
  virtual void DrawGizmos(NodeValueDatabase& db, QPainter *p, const QVector2D &scale, const QSize& viewport) const override;

  virtual bool GizmoPress(NodeValueDatabase& db, const QPointF &p, const QVector2D &scale, const QSize& viewport) override;
  virtual void GizmoMove(const QPointF &p, const QVector2D &scale, const rational &time) override;
  virtual void GizmoRelease() override;

private:
  struct GizmoSharedData {
    GizmoSharedData(const QSize& viewport, const QVector2D& scale);

    QPointF half_viewport;
    QVector2D half_scale;
    QVector2D inverted_half_scale;
  };

  QMatrix4x4 GenerateMatrix(NodeValueDatabase& value) const;
  QMatrix4x4 GenerateMatrix(NodeValueDatabase &value, bool ignore_anchor) const;
  static QMatrix4x4 GenerateMatrix(const QVector2D &pos,
                                   const float &rot,
                                   const QVector2D &scale,
                                   bool uniform_scale,
                                   const QVector2D &anchor);

  QPointF GetGizmoAnchorPoint(NodeValueDatabase &db, const GizmoSharedData &gizmo_data) const;
  static int GetGizmoAnchorPointRadius();
  NodeInput* gizmo_drag_;

  NodeInput* position_input_;

  NodeInput* rotation_input_;

  NodeInput* scale_input_;

  NodeInput* uniform_scale_input_;

  NodeInput* anchor_input_;

  QVector2D gizmo_start_;
  QVector2D gizmo_start2_;
  QPointF gizmo_mouse_start_;
  NodeInputDragger gizmo_x_dragger_;
  NodeInputDragger gizmo_y_dragger_;
  NodeInputDragger gizmo_x2_dragger_;
  NodeInputDragger gizmo_y2_dragger_;

private slots:
  void UniformScaleChanged();

};

OLIVE_NAMESPACE_EXIT

#endif // TRANSFORMDISTORT_H
