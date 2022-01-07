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

#ifndef NODEVIEWUNDO_H
#define NODEVIEWUNDO_H

#include "node/graph.h"
#include "node/node.h"
#include "node/project/project.h"
#include "undo/undocommand.h"

namespace olive {

/**
 * @brief An undoable command for disconnecting two NodeParams
 *
 * Can be considered a UndoCommand wrapper for NodeParam::DisonnectEdge()/
 */
class NodeEdgeRemoveCommand : public UndoCommand {
public:
  NodeEdgeRemoveCommand(Node *output, const NodeInput& input);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  Node *output_;
  NodeInput input_;

};

/**
 * @brief An undoable command for connecting two NodeParams together
 *
 * Can be considered a UndoCommand wrapper for NodeParam::ConnectEdge()/
 */
class NodeEdgeAddCommand : public UndoCommand {
public:
  NodeEdgeAddCommand(Node *output, const NodeInput& input);

  virtual ~NodeEdgeAddCommand() override;

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  Node *output_;
  NodeInput input_;

  NodeEdgeRemoveCommand* remove_command_;

};

class NodeAddCommand : public UndoCommand {
public:
  NodeAddCommand(NodeGraph* graph, Node* node);

  void PushToThread(QThread* thread);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  QObject memory_manager_;

  NodeGraph* graph_;
  Node* node_;
};

class NodeRemoveAndDisconnectCommand : public UndoCommand {
public:
  NodeRemoveAndDisconnectCommand(Node* node) :
    node_(node),
    graph_(nullptr),
    command_(nullptr)
  {
  }

  virtual ~NodeRemoveAndDisconnectCommand() override
  {
    delete command_;
  }

  virtual Project* GetRelevantProject() const override
  {
    return dynamic_cast<Project*>(graph_);
  }

protected:
  virtual void prepare() override;

  virtual void redo() override
  {
    command_->redo_now();

    graph_ = node_->parent();
    node_->setParent(&memory_manager_);
  }

  virtual void undo() override
  {
    node_->setParent(graph_);
    graph_ = nullptr;

    command_->undo_now();
  }

private:
  QObject memory_manager_;

  Node* node_;
  NodeGraph* graph_;

  MultiUndoCommand* command_;

};

class NodeRemoveWithExclusiveDependenciesAndDisconnect : public UndoCommand {
public:
  NodeRemoveWithExclusiveDependenciesAndDisconnect(Node* node) :
    node_(node),
    command_(nullptr)
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
      return node_->project();
    }
  }

protected:
  virtual void prepare() override
  {
    command_ = new MultiUndoCommand();

    command_->add_child(new NodeRemoveAndDisconnectCommand(node_));

    // Remove exclusive dependencies
    QVector<Node*> deps = node_->GetExclusiveDependencies();
    foreach (Node* d, deps) {
      command_->add_child(new NodeRemoveAndDisconnectCommand(d));
    }
  }

  virtual void redo() override
  {
    command_->redo_now();
  }

  virtual void undo() override
  {
    command_->undo_now();
  }

private:
  Node* node_;
  MultiUndoCommand* command_;

};

class NodeCopyInputsCommand : public UndoCommand {
public:
  NodeCopyInputsCommand(const Node* src,
                        Node* dest,
                        bool include_connections);

  virtual Project* GetRelevantProject() const override {return nullptr;}

protected:
  virtual void redo() override;

  virtual void undo() override {}

private:
  const Node* src_;

  Node* dest_;

  bool include_connections_;

};

class NodeLinkCommand : public UndoCommand {
public:
  NodeLinkCommand(Node* a, Node* b, bool link) :
    a_(a),
    b_(b),
    link_(link)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return a_->project();
  }

protected:
  virtual void redo() override
  {
    if (link_) {
      done_ = Node::Link(a_, b_);
    } else {
      done_ = Node::Unlink(a_, b_);
    }
  }

  virtual void undo() override
  {
    if (done_) {
      if (link_) {
        Node::Unlink(a_, b_);
      } else {
        Node::Link(a_, b_);
      }
    }
  }

private:
  Node* a_;
  Node* b_;
  bool link_;
  bool done_;

};

class NodeUnlinkAllCommand : public UndoCommand {
public:
  NodeUnlinkAllCommand(Node* node) :
    node_(node)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return node_->project();
  }

protected:
  virtual void redo() override
  {
    unlinked_ = node_->links();

    foreach (Node* link, unlinked_) {
      Node::Unlink(node_, link);
    }
  }

  virtual void undo() override
  {
    foreach (Node* link, unlinked_) {
      Node::Link(node_, link);
    }

    unlinked_.clear();
  }

private:
  Node* node_;

  QVector<Node*> unlinked_;

};

class NodeLinkManyCommand : public MultiUndoCommand {
public:
  NodeLinkManyCommand(const QVector<Node*> nodes, bool link) :
    nodes_(nodes)
  {
    foreach (Node* a, nodes_) {
      foreach (Node* b, nodes_) {
        if (a != b) {
          add_child(new NodeLinkCommand(a, b, link));
        }
      }
    }
  }

  virtual Project* GetRelevantProject() const override
  {
    return nodes_.first()->project();
  }

private:
  QVector<Node*> nodes_;

};

class NodeRenameCommand : public UndoCommand
{
public:
  NodeRenameCommand() = default;

  void AddNode(Node* node, const QString& new_name);

  virtual Project * GetRelevantProject() const override;

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  QVector<Node*> nodes_;

  QStringList new_labels_;
  QStringList old_labels_;

};

class NodeOverrideColorCommand : public UndoCommand
{
public:
  NodeOverrideColorCommand(Node *node, int index);

  virtual Project * GetRelevantProject() const override;

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  Node *node_;

  int old_index_;

  int new_index_;

};

class NodeViewDeleteCommand : public UndoCommand
{
public:
  NodeViewDeleteCommand();

  void AddNode(Node *node, Node *context);

  void AddEdge(Node *output, const NodeInput &input);

  virtual Project * GetRelevantProject() const override;

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  using NodePair = QPair<Node *, Node*>;

  QVector<NodePair> nodes_;

  QVector<Node::OutputConnection> edges_;

  struct RemovedNode {
    Node *node;
    Node *context;
    QPointF pos;
    NodeGraph *removed_from_graph;
  };

  QVector<RemovedNode> removed_nodes_;

  QObject memory_manager_;

};

}

#endif // NODEVIEWUNDO_H
