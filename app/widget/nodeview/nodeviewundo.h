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

#ifndef NODEVIEWUNDO_H
#define NODEVIEWUNDO_H

#include <QUndoCommand>

#include "node/graph.h"
#include "node/node.h"
#include "undo/undocommand.h"

namespace olive {

/**
 * @brief An undoable command for connecting two NodeParams together
 *
 * Can be considered a QUndoCommand wrapper for NodeParam::ConnectEdge()/
 */
class NodeEdgeAddCommand : public UndoCommand {
public:
  NodeEdgeAddCommand(Node* output, NodeInput* input, int element, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  Node* output_;
  NodeInput* input_;
  int element_;

};

/**
 * @brief An undoable command for disconnecting two NodeParams
 *
 * Can be considered a QUndoCommand wrapper for NodeParam::DisonnectEdge()/
 */
class NodeEdgeRemoveCommand : public UndoCommand {
public:
  NodeEdgeRemoveCommand(Node* output, NodeInput* input, int element, QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  Node* output_;
  NodeInput* input_;
  int element_;

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

class NodeRemoveAndDisconnectCommand : public UndoCommand {
public:
  NodeRemoveAndDisconnectCommand(Node* node, QUndoCommand* parent = nullptr) :
    UndoCommand(parent),
    node_(node),
    graph_(nullptr),
    command_(nullptr),
    prepped_(false)
  {
  }

  virtual ~NodeRemoveAndDisconnectCommand() override
  {
    delete command_;
  }

  virtual Project* GetRelevantProject() const override
  {
    if (graph_) {
      return graph_->project();
    } else {
      return node_->parent()->project();
    }
  }

protected:
  virtual void redo_internal() override
  {
    if (!prepped_) {
      prep();
      prepped_ = true;
    }

    command_->redo();

    graph_ = node_->parent();
    node_->setParent(&memory_manager_);
  }

  virtual void undo_internal() override
  {
    node_->setParent(graph_);
    graph_ = nullptr;

    command_->undo();
  }

private:
  void prep();

  QObject memory_manager_;

  Node* node_;
  NodeGraph* graph_;

  QUndoCommand* command_;

  bool prepped_;

};

class NodeRemoveWithExclusiveDependenciesAndDisconnect : public UndoCommand {
public:
  NodeRemoveWithExclusiveDependenciesAndDisconnect(Node* node, QUndoCommand* parent = nullptr) :
    UndoCommand(parent),
    node_(node),
    command_(nullptr),
    prepped_(false)
  {
  }

  virtual ~NodeRemoveWithExclusiveDependenciesAndDisconnect() override
  {
    delete command_;
  }

  virtual Project* GetRelevantProject() const override
  {
    if (command_) {
      return static_cast<const NodeRemoveAndDisconnectCommand*>(command_->child(0))->GetRelevantProject();
    } else {
      return node_->parent()->project();
    }
  }

protected:
  virtual void redo_internal() override
  {
    if (!prepped_) {
      prep();
      prepped_ = true;
    }

    command_->redo();
  }

  virtual void undo_internal() override
  {
    command_->undo();
  }

private:
  void prep()
  {
    command_ = new QUndoCommand();

    new NodeRemoveAndDisconnectCommand(node_, command_);

    // Remove exclusive dependencies
    QVector<Node*> deps = node_->GetExclusiveDependencies();
    foreach (Node* d, deps) {
      new NodeRemoveAndDisconnectCommand(d, command_);
    }
  }

  Node* node_;
  QUndoCommand* command_;
  bool prepped_;

};

class NodeCopyInputsCommand : public QUndoCommand {
public:
  NodeCopyInputsCommand(Node* src,
                        Node* dest,
                        bool include_connections,
                        QUndoCommand* parent = nullptr);

protected:
  virtual void redo() override;

private:
  Node* src_;

  Node* dest_;

  bool include_connections_;

};

}

#endif // NODEVIEWUNDO_H
