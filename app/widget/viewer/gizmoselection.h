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

#ifndef GIZMOSELECTION_H
#define GIZMOSELECTION_H

#include "node/gizmo/gizmo.h"
#include "node/node.h"

class QMouseEvent;
class QKeyEvent;
class QPainterPath;

namespace olive {


class GizmoSelection : public QObject
{
  Q_OBJECT
public:
  GizmoSelection( QWidget * owner,
                  QTransform & last_draw_transform_);

  void SetGizmos( Node* gizmos) {
     gizmos_ = gizmos;
  }

  const QList<NodeGizmo *> & SelectedGizmos() const {
    return selected_gizmos_;
  }

  const NodeGizmo * CurrentGizmo() const {
    return current_gizmo_;
  }

  void OnMouseLeftPress(QMouseEvent *event);

  void OnMouseRelease(QMouseEvent *event);

  void OnMouseMove(QMouseEvent *event);

  void DrawSelection(QPainter & painter);

private:
  void onMouseHover(QMouseEvent *event);
  void deselectAllGizmos();
  void selectAllGizmos();
  void addToSelection(NodeGizmo * gizmo, bool prepend = false);
  QList<NodeGizmo *> tryGizmoPress( const QPointF &p);
  void startLassoSelection(QMouseEvent *event);

private:
  QWidget * owner_;
  QTransform & gizmo_last_draw_transform_;
  NodeGizmo * current_gizmo_;
  NodeGizmo * hovered_gizmo_;
  QList<NodeGizmo *> selected_gizmos_;
  Node* gizmos_;
  QPainterPath * lasso_path_;
  bool draw_lasso_flag_;
};

}  // namespace olive

#endif // GIZMOSELECTION_H
