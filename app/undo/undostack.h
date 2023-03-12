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

#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include <QAction>
#include <QAbstractItemModel>

#include "common/define.h"
#include "undo/undocommand.h"

namespace olive {

class UndoStack : public QAbstractItemModel
{
  Q_OBJECT
public:
  UndoStack();

  virtual ~UndoStack() override;

  void push(UndoCommand* command);

  void jump(size_t index);

  void clear();

  bool CanUndo() const;

  bool CanRedo() const
  {
    return !undone_commands_.empty();
  }

  void UpdateActions();

  QAction* GetUndoAction()
  {
    return undo_action_;
  }

  QAction* GetRedoAction()
  {
    return redo_action_;
  }

  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex &index) const override;
  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

signals:
  void indexChanged(int i);

public slots:
  void undo();

  void redo();

private:
  static const int kMaxUndoCommands;

  std::list<UndoCommand*> commands_;

  std::list<UndoCommand*> undone_commands_;

  QAction* undo_action_;

  QAction* redo_action_;

};

}

#endif // UNDOSTACK_H
