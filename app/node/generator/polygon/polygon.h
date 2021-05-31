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

#ifndef POLYGONGENERATOR_H
#define POLYGONGENERATOR_H

#include "node/node.h"
#include "node/inputdragger.h"

namespace olive {

class PolygonGenerator : public Node
{
  Q_OBJECT
public:
  PolygonGenerator();

  NODE_DEFAULT_DESTRUCTOR(PolygonGenerator)

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual ShaderCode GetShaderCode(const QString& shader_id) const override;
  virtual NodeValueTable Value(const QString& output, NodeValueDatabase &value) const override;

  virtual bool HasGizmos() const override;
  //virtual void DrawGizmos(NodeValueDatabase& db, QPainter *p) const override;

  //virtual bool GizmoPress(NodeValueDatabase &db, const QPointF &p) override;
  //virtual void GizmoMove(const QPointF &p, const QVector2D &scale, const rational &time) override;
  //virtual void GizmoRelease() override;

  static const QString kPointsInput;
  static const QString kColorInput;

private:
  QVector<QPointF> GetGizmoCoordinates(NodeValueDatabase &db, const QVector2D &scale) const;

  QVector<QRectF> GetGizmoRects(const QVector<QPointF>& points) const;

  NodeInput* gizmo_drag_;
  QPointF gizmo_drag_start_;

  NodeInputDragger gizmo_x_dragger_;
  NodeInputDragger gizmo_y_dragger_;

};

}

#endif // POLYGONGENERATOR_H
