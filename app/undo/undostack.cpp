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

#include "undostack.h"

#include <QCoreApplication>

namespace olive {

const int UndoStack::kMaxUndoCommands = 200;

UndoStack::UndoStack()
{
  undo_action_ = new QAction();
  connect(undo_action_, &QAction::triggered, this, &UndoStack::undo);

  redo_action_ = new QAction();
  connect(redo_action_, &QAction::triggered, this, &UndoStack::redo);

  UpdateActions();
}

UndoStack::~UndoStack()
{
  clear();

  delete undo_action_;
  delete redo_action_;
}

void UndoStack::pushIfHasChildren(MultiUndoCommand *command)
{
  if (command->child_count() > 0) {
    push(command);
  } else {
    delete command;
  }
}

void UndoStack::push(UndoCommand *command)
{
  command->redo_and_set_modified();
  commands_.push_back(command);

  if (commands_.size() > kMaxUndoCommands) {
    delete commands_.front();
    commands_.pop_front();
  }

  if (CanRedo()) {
    for (auto it=undone_commands_.cbegin(); it!=undone_commands_.cend(); it++) {
      delete (*it);
    }
    undone_commands_.clear();
  }

  UpdateActions();
}

void UndoStack::undo()
{
  if (CanUndo()) {
    // Undo most recently done command
    commands_.back()->undo_and_set_modified();

    // Place at the front of the "undone commands" list
    undone_commands_.push_front(commands_.back());

    // Remove undone command from the commands list
    commands_.pop_back();

    // Update actions
    UpdateActions();
  }
}

void UndoStack::redo()
{
  if (CanRedo()) {
    // Redo most recently undone command
    undone_commands_.front()->redo_and_set_modified();

    // Place at the back of the done commands list
    commands_.push_back(undone_commands_.front());

    // Remove done command from undone list
    undone_commands_.pop_front();

    // Update actions
    UpdateActions();
  }
}

void UndoStack::clear()
{
  for (auto it=commands_.cbegin(); it!=commands_.cend(); it++) {
    delete (*it);
  }
  commands_.clear();
}

void UndoStack::UpdateActions()
{
  undo_action_->setEnabled(CanUndo());
  redo_action_->setEnabled(CanRedo());

  undo_action_->setText(QCoreApplication::translate("UndoStack", "Undo %1").arg(CanUndo() ? commands_.back()->name() : QString()));
  redo_action_->setText(QCoreApplication::translate("UndoStack", "Redo %1").arg(CanRedo() ? undone_commands_.front()->name() : QString()));
}

}
