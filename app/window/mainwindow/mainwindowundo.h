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

#ifndef MAINWINDOWUNDO_H
#define MAINWINDOWUNDO_H

#include "project/item/sequence/sequence.h"

namespace olive {

class OpenSequenceCommand : public UndoCommand
{
public:
  OpenSequenceCommand(Sequence* sequence) :
    sequence_(sequence)
  {}

  virtual void redo() override;

  virtual void undo() override;

  virtual Project* GetRelevantProject() const override {return nullptr;}

private:
  Sequence* sequence_;

};

class CloseSequenceCommand : public UndoCommand
{
public:
  CloseSequenceCommand(Sequence* sequence) :
    sequence_(sequence)
  {}

  virtual void redo() override;

  virtual void undo() override;

  virtual Project* GetRelevantProject() const override {return nullptr;}

private:
  Sequence* sequence_;

};

}

#endif // MAINWINDOWUNDO_H
