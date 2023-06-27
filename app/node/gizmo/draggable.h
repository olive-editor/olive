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

#ifndef DRAGGABLEGIZMO_H
#define DRAGGABLEGIZMO_H

#include "gizmo.h"
#include "node/inputdragger.h"
#include "undo/undocommand.h"

namespace olive {

class DraggableGizmo : public NodeGizmo
{
  Q_OBJECT
public:
  /// Changes what the X/Y coordinates emitted from HandleMovement specify
  enum DragValueBehavior {
    /// X/Y will be the exact mouse coordinates (in sequence pixels)
    kAbsolute,

    /// X/Y will be the movement since the last time HandleMovement was called
    kDeltaFromPrevious,

    /// X/Y will be the movement from the start of the drag
    kDeltaFromStart
  };

  explicit DraggableGizmo(QObject *parent = nullptr);

  void DragStart(const NodeValueRow &row, double abs_x, double abs_y, const rational &time);

  void DragMove(double x, double y, const Qt::KeyboardModifiers &modifiers);

  void DragEnd(olive::MultiUndoCommand *command);

  void AddInput(const NodeKeyframeTrackReference &input)
  {
    inputs_.append(input);
    draggers_.append(NodeInputDragger());
  }

  QVector<NodeInputDragger> &GetDraggers()
  {
    return draggers_;
  }

  DragValueBehavior GetDragValueBehavior() const { return drag_value_behavior_; }
  void SetDragValueBehavior(DragValueBehavior d) { drag_value_behavior_ = d; }

signals:
  void HandleStart(const olive::NodeValueRow &row, double x, double y, const rational &time);

  void HandleMovement(double x, double y, const Qt::KeyboardModifiers &modifiers);

private:
  QVector<NodeKeyframeTrackReference> inputs_;

  QVector<NodeInputDragger> draggers_;

  DragValueBehavior drag_value_behavior_;

};

}

#endif // DRAGGABLEGIZMO_H
