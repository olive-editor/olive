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

#include <QPainterPath>

#include "common/bezier.h"
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

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  virtual void GenerateFrame(FramePtr frame, const GenerateJob &job) const override;

  virtual bool HasGizmos() const override;
  virtual void DrawGizmos(const NodeValueRow& row, const NodeGlobals &globals, QPainter *p) override;

  virtual bool GizmoPress(const NodeValueRow& row, const NodeGlobals &globals, const QPointF &p) override;
  virtual void GizmoMove(const QPointF &p, const rational &time, const Qt::KeyboardModifiers &modifiers) override;
  virtual void GizmoRelease(MultiUndoCommand *command) override;

  static const QString kPointsInput;
  static const QString kColorInput;

private:
  static void AddPointToPath(QPainterPath *path, const Bezier &before, const Bezier &after);

  static QPainterPath GeneratePath(const QVector<NodeValue> &points);

  QPainterPath gizmo_polygon_path_;
  QVector<QRectF> gizmo_position_handles_;
  QVector<QRectF> gizmo_bezier_handles_;

  QVector<NodeKeyframeTrackReference> gizmo_x_active_;
  QVector<NodeKeyframeTrackReference> gizmo_y_active_;
  QVector<NodeInputDragger> gizmo_x_draggers_;
  QVector<NodeInputDragger> gizmo_y_draggers_;
  QPointF gizmo_drag_start_;

};

}

#endif // POLYGONGENERATOR_H
