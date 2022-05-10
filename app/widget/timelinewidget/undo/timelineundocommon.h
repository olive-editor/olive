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

#ifndef TIMELINEUNDOCOMMON_H
#define TIMELINEUNDOCOMMON_H

#include "node/node.h"
#include "widget/nodeview/nodeviewundo.h"

namespace olive {

inline bool NodeCanBeRemoved(Node* n)
{
  return n->output_connections().empty();
}

inline UndoCommand* CreateRemoveCommand(Node* n)
{
  return new NodeRemoveWithExclusiveDependenciesAndDisconnect(n);
}

inline UndoCommand* CreateAndRunRemoveCommand(Node* n)
{
  UndoCommand* command = CreateRemoveCommand(n);
  command->redo_now();
  return command;
}

}

#endif // TIMELINEUNDOCOMMON_H
