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

#ifndef NODEGROUPDIALOG_H
#define NODEGROUPDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QMainWindow>

#include "node/group/group.h"
#include "undo/undostack.h"

namespace olive {

class NodeGroupDialog : public QDialog
{
  Q_OBJECT
public:
  explicit NodeGroupDialog(NodeGroup *group, QWidget *parent = nullptr);

  void PrependUndoCommand(MultiUndoCommand *c)
  {
    prepend_undo_ = c;
  }

public slots:
  virtual void accept() override;

  virtual void reject() override;

signals:

private:
  QVector<Node*> GetNodesToDelete();

  QVector<Node::OutputConnection> GetNewConnections();

  bool OperationWillAffectOutsideGroup(const QVector<Node *> &deleted, const QVector<Node::OutputConnection> &connections);

  NodeGroup *group_;

  QLineEdit *name_edit_;

  MultiUndoCommand *prepend_undo_;

  Project *copied_project_;
  NodeGroup *copy_group_;
  QMap<Node*, Node*> copy_subnodes_;
  QVector<Node::OutputConnection> copied_edges_;

  UndoStack undo_stack_;

};

}

#endif // NODEGROUPDIALOG_H
