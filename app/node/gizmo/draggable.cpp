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

#include "draggable.h"

namespace olive {

DraggableGizmo::DraggableGizmo(QObject *parent)
  : NodeGizmo{parent},
    drag_value_behavior_(kAbsolute)
{
}

void DraggableGizmo::DragStart(const NodeValueRow &row, double abs_x, double abs_y, const rational &time)
{
  for (int i=0; i<draggers_.size(); i++) {
    draggers_[i].Start(inputs_[i], time);
  }

  emit HandleStart(row, abs_x, abs_y, time);
}

void DraggableGizmo::DragMove(double x, double y, const Qt::KeyboardModifiers &modifiers)
{
  emit HandleMovement(x, y, modifiers);
}

void DraggableGizmo::DragEnd(MultiUndoCommand *command)
{
  for (int i=0; i<draggers_.size(); i++) {
    draggers_[i].End(command);
  }
}

}
