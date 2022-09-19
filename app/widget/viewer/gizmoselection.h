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

/// This class handles the mouse operations over a gizmo or set of gizmos
/// to modify the selection. In particular:
/// * LEFT_CLICK on gizmo => select the gizmo. If control points and main point
///         are stacked at the same location, the main point is always selected.
///         Previously selected gizmos are deselected
/// * ALT+LEFT_CLICK on gizmo => same as 'LEFT_CLICK', but when control points are in the same location
///         as main point, one control point is selected
/// * SHIFT+LEFT_CLICK => toggles the selection of the clicked gizmos and preserves
///         prevously selected gizmos
/// * LEFT_CLICK outside any gizmo => deselect all gizmos
/// * CTRL+LEFT_CLICK outside any gizmo => select all gizmos
/// * Lasso => select gizmos inside lasso. All gizmos outside lasso are deselected
/// * SHIFT+Lasso => toggle the selection state of gizmos inside lasso. The selection
///         state of all gizmos outside lasso is unchanged.
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

  const NodeGizmo * PressedGizmo() const {
    return pressed_gizmo_;
  }

  void OnMouseLeftPress(QMouseEvent *event);

  void OnMouseRelease(QMouseEvent *event);

  void OnMouseMove(QMouseEvent *event);

  void DrawSelection(QPainter & painter);

  bool CanMoveGizmo( NodeGizmo * gizmo) const;

private:
  void onMouseHover(QMouseEvent *event);
  void deselectAllGizmos();
  void addToSelection(NodeGizmo * gizmo);
  void toggleSelection(NodeGizmo * gizmo);
  QList<NodeGizmo *> tryPointInGizmo( const QPointF &p);
  void startLassoSelection(QMouseEvent *event);
  void selectGizmosInsideLasso(bool toggle);

private:
  QWidget * owner_;
  QTransform & gizmo_last_draw_transform_;
  NodeGizmo * pressed_gizmo_;
  NodeGizmo * hovered_gizmo_;
  QList<NodeGizmo *> selected_gizmos_;
  Node* gizmos_;
  QPainterPath * lasso_path_;
  bool draw_lasso_flag_;

};

}  // namespace olive

#endif // GIZMOSELECTION_H
