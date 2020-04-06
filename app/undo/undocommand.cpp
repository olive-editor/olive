/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "undocommand.h"

#include "core.h"

OLIVE_NAMESPACE_ENTER

UndoCommand::UndoCommand(QUndoCommand *parent) :
  QUndoCommand(parent)
{
}

void UndoCommand::redo()
{
  redo_internal();

  modified_ = Core::instance()->IsProjectModified();
  Core::instance()->SetProjectModified(true);
}

void UndoCommand::undo()
{
  undo_internal();

  Core::instance()->SetProjectModified(modified_);
}

void UndoCommand::redo_internal()
{
  QUndoCommand::redo();
}

void UndoCommand::undo_internal()
{
  QUndoCommand::undo();
}

OLIVE_NAMESPACE_EXIT
