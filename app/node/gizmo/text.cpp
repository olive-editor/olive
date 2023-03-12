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

#include "text.h"

#include "core.h"
#include "undo/undocommand.h"

namespace olive {

TextGizmo::TextGizmo(QObject *parent)
  : NodeGizmo{parent},
    valign_(Qt::AlignTop)
{

}

void TextGizmo::SetRect(const QRectF &r)
{
  rect_ = r;
  emit RectChanged(rect_);
}

void TextGizmo::UpdateInputHtml(const QString &s, const rational &time)
{
  if (input_.IsValid()) {
    MultiUndoCommand *command = new MultiUndoCommand();
    Node::SetValueAtTime(input_.input(), time, s, input_.track(), command, true);
    Core::instance()->undo_stack()->push(command, tr("Edit Text"));
  }
}

void TextGizmo::SetVerticalAlignment(Qt::Alignment va)
{
  valign_ = va;
  emit VerticalAlignmentChanged(valign_);
}

}
