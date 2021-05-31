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

#ifndef PREFERENCESTAB_H
#define PREFERENCESTAB_H

#include <QWidget>

#include "config/config.h"
#include "undo/undocommand.h"

namespace olive {

class ConfigDialogBaseTab : public QWidget
{
public:
  ConfigDialogBaseTab() = default;

  virtual bool Validate();

  virtual void Accept(MultiUndoCommand *parent) = 0;
};

}

#endif // PREFERENCESTAB_H
