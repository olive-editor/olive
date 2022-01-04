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

#ifndef SHAPENODEBASE_H
#define SHAPENODEBASE_H

#include "node/inputdragger.h"
#include "node/node.h"

namespace olive {

class ShapeNodeBase : public Node
{
  Q_OBJECT
public:
  ShapeNodeBase();

  NODE_DEFAULT_DESTRUCTOR(ShapeNodeBase)

  virtual void Retranslate() override;

  static QString kPositionInput;
  static QString kSizeInput;
  static QString kColorInput;

  virtual bool HasGizmos() const override
  {
    return true;
  }

  virtual void DrawGizmos(const NodeValueRow &row, const NodeGlobals &globals, QPainter *p) override;

  virtual bool GizmoPress(const NodeValueRow& row, const NodeGlobals &globals, const QPointF &p) override;
  virtual void GizmoMove(const QPointF &p, const rational &time, const Qt::KeyboardModifiers &modifiers) override;
  virtual void GizmoRelease(MultiUndoCommand *command) override;

private:
  static QVector2D GenerateGizmoAnchor(const QVector2D &pos, const QVector2D &size, int drag, QVector2D *pt);

  // Gizmo variables
  static const int kGizmoWholeRect = kGizmoScaleCount;
  QRectF gizmo_resize_handle_[kGizmoScaleCount];
  QRectF gizmo_whole_rect_;

  int gizmo_drag_;
  QVector<NodeInputDragger> gizmo_dragger_;
  QVector2D gizmo_pos_start_;
  QVector2D gizmo_sz_start_;
  QPointF gizmo_drag_start_;
  float gizmo_aspect_ratio_;
  QVector2D gizmo_half_res_;

};

}

#endif // SHAPENODEBASE_H
