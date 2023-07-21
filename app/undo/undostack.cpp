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

#include "undostack.h"

#include <QCoreApplication>

namespace olive {

const int UndoStack::kMaxUndoCommands = 200;

class EmptyCommand : public UndoCommand
{
public:
  EmptyCommand(){}

  virtual Project* GetRelevantProject() const override {return nullptr;}

protected:
  virtual void redo() override {}
  virtual void undo() override {}

};

UndoStack::UndoStack()
{
  undo_action_ = new QAction();
  connect(undo_action_, &QAction::triggered, this, &UndoStack::undo);

  redo_action_ = new QAction();
  connect(redo_action_, &QAction::triggered, this, &UndoStack::redo);

  clear();
  UpdateActions();
}

UndoStack::~UndoStack()
{
  clear();

  delete undo_action_;
  delete redo_action_;
}

void UndoStack::push(UndoCommand *command, const QString &name)
{
  MultiUndoCommand *mcu = dynamic_cast<MultiUndoCommand*>(command);
  if (mcu && mcu->child_count() == 0) {
    delete command;
    return;
  }

  // Clear any redoable commands
  this->beginRemoveRows(QModelIndex(), commands_.size(), commands_.size() + undone_commands_.size());
  if (CanRedo()) {
    for (auto it=undone_commands_.cbegin(); it!=undone_commands_.cend(); it++) {
      delete (*it).command;
    }
    undone_commands_.clear();
  }
  this->endRemoveRows();

  // Do command and push
  this->beginInsertRows(QModelIndex(), commands_.size(), commands_.size());
  command->redo_and_set_modified();
  commands_.push_back({command, name});
  this->endInsertRows();

  // Delete oldest
  if (commands_.size() > kMaxUndoCommands) {
    this->beginRemoveRows(QModelIndex(), 0, 0);
    delete commands_.front().command;
    commands_.pop_front();
    this->endRemoveRows();
  }

  UpdateActions();
}

void UndoStack::jump(size_t index)
{
  while (commands_.size() > index) {
    undo();
  }
  while (commands_.size() < index) {
    redo();
  }
}

void UndoStack::undo()
{
  if (CanUndo()) {
    // Undo most recently done command
    commands_.back().command->undo_and_set_modified();

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
    undone_commands_.front().command->redo_and_set_modified();

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
  this->beginResetModel();

  for (auto it=commands_.cbegin(); it!=commands_.cend(); it++) {
    delete (*it).command;
  }
  commands_.clear();
  for (auto it=undone_commands_.cbegin(); it!=undone_commands_.cend(); it++) {
    delete (*it).command;
  }
  undone_commands_.clear();

  this->endResetModel();

  push(new EmptyCommand(), tr("New/Open Project"));
}

bool UndoStack::CanUndo() const
{
  return !commands_.empty() && !dynamic_cast<EmptyCommand*>(commands_.back().command);
}

void UndoStack::UpdateActions()
{
  undo_action_->setEnabled(CanUndo());
  redo_action_->setEnabled(CanRedo());

  undo_action_->setText(QCoreApplication::translate("UndoStack", "Undo %1").arg(CanUndo() ? commands_.back().name : QString()));
  redo_action_->setText(QCoreApplication::translate("UndoStack", "Redo %1").arg(CanRedo() ? undone_commands_.front().name : QString()));

  emit indexChanged(commands_.size());
}

int UndoStack::columnCount(const QModelIndex &parent) const
{
  if (parent.isValid()) {
    return 0;
  }
  return 2;
}

QVariant UndoStack::data(const QModelIndex &index, int role) const
{
  if (role == Qt::DisplayRole) {
    switch (index.column()) {
    case 0:
      return index.row() + 1;
    case 1:
    {
      std::list<CommandEntry>::const_iterator it;
      size_t real_index = index.row();
      if (real_index < commands_.size()) {
        it = commands_.cbegin();
      } else {
        real_index -= commands_.size();
        it = undone_commands_.cbegin();
      }
      for (size_t i = 0; i < real_index; i++) {
        it++;
      }
      const QString &name = (*it).name;
      return (name.isEmpty()) ? tr("Command") : name;
    }
    }
  } else if (role == Qt::ForegroundRole) {
    if (size_t(index.row()) >= commands_.size()) {
      return QVariant(QColor(Qt::gray));
    }
  }

  return QVariant();
}

QModelIndex UndoStack::index(int row, int column, const QModelIndex &parent) const
{
  return createIndex(row, column, nullptr);
}

QModelIndex UndoStack::parent(const QModelIndex &index) const
{
  return QModelIndex();
}

int UndoStack::rowCount(const QModelIndex &parent) const
{
  if (parent.isValid()) {
    return 0;
  }

  return commands_.size() + undone_commands_.size();
}

QVariant UndoStack::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole) {
    switch (section) {
    case 0:
      return QStringLiteral("Number");
    case 1:
      return QStringLiteral("Action");
    }
  }

  return QVariant();
}

bool UndoStack::hasChildren(const QModelIndex &parent) const
{
  return !parent.isValid();
}

}
