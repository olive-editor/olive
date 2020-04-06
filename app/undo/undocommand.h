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

#ifndef UNDOCOMMAND_H
#define UNDOCOMMAND_H

#include <QUndoCommand>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class UndoCommand : public QUndoCommand
{
public:
  UndoCommand(QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

protected:
  virtual void redo_internal();
  virtual void undo_internal();

private:
  bool modified_;

};

OLIVE_NAMESPACE_EXIT

#endif // UNDOCOMMAND_H
