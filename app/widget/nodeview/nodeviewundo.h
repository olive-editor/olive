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

#ifndef NODEVIEWUNDO_H
#define NODEVIEWUNDO_H

#include <QUndoCommand>

#include "node/graph.h"
#include "node/node.h"
#include "nodeviewitem.h"
#include "undo/undocommand.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief An undoable command for connecting two NodeParams together
 *
 * Can be considered a QUndoCommand wrapper for NodeParam::ConnectEdge()/
 */
class NodeEdgeAddCommand : public UndoCommand {
public:
  NodeEdgeAddCommand(NodeOutput* output, NodeInput* input, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  NodeOutput* output_;
  NodeInput* input_;

  NodeEdgePtr old_edge_;

  bool done_;
};

/**
 * @brief An undoable command for disconnecting two NodeParams
 *
 * Can be considered a QUndoCommand wrapper for NodeParam::DisonnectEdge()/
 */
class NodeEdgeRemoveCommand : public UndoCommand {
public:
  NodeEdgeRemoveCommand(NodeOutput* output, NodeInput* input, QUndoCommand* parent = nullptr);
  NodeEdgeRemoveCommand(NodeEdgePtr edge, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  NodeOutput* output_;
  NodeInput* input_;

  bool done_;
};

class NodeAddCommand : public UndoCommand {
public:
  NodeAddCommand(NodeGraph* graph, Node* node, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  QObject memory_manager_;

  NodeGraph* graph_;
  Node* node_;
};

class NodeRemoveCommand : public UndoCommand {
public:
  NodeRemoveCommand(NodeGraph* graph,
                    const QList<Node*>& nodes,
                    QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  QObject memory_manager_;

  NodeGraph* graph_;
  QList<Node*> nodes_;
  QList<NodeEdgePtr> edges_;
};

class NodeRemoveWithExclusiveDeps : public UndoCommand {
public:
  NodeRemoveWithExclusiveDeps(NodeGraph* graph,
                              Node* node,
                              QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

private:
  NodeRemoveCommand* remove_command_;
};

class NodeCopyInputsCommand : public QUndoCommand {
public:
  NodeCopyInputsCommand(Node* src,
                        Node* dest,
                        bool include_connections = true,
                        QUndoCommand* parent = nullptr);

  NodeCopyInputsCommand(Node* src,
                        Node* dest,
                        QUndoCommand* parent = nullptr);

protected:
  virtual void redo() override;

private:
  Node* src_;

  Node* dest_;

  bool include_connections_;

};

OLIVE_NAMESPACE_EXIT

#endif // NODEVIEWUNDO_H
