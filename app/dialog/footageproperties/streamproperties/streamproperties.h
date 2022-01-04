/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef STREAMPROPERTIES_H
#define STREAMPROPERTIES_H

#include <QWidget>

#include "common/define.h"
#include "undo/undocommand.h"

namespace olive {

class StreamProperties : public QWidget
{
public:
  StreamProperties(QWidget* parent = nullptr);

  virtual void Accept(MultiUndoCommand*){}

  virtual bool SanityCheck(){return true;}

};

}

#endif // STREAMPROPERTIES_H
