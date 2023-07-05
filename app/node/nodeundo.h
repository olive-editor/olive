/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#ifndef NODEUNDO_H
#define NODEUNDO_H

#include "node/node.h"
#include "node/project.h"
#include "undo/undocommand.h"

namespace olive {

class NodeSetPositionCommand : public UndoCommand
{
public:
  NodeSetPositionCommand(Node* node, Node* context, const Node::Position& pos)
  {
    node_ = node;
    context_ = context;
    pos_ = pos;
  }

  virtual Project* GetRelevantProject() const override
  {
    return node_->project();
  }

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  Node* node_;
  Node* context_;
  Node::Position pos_;
  Node::Position old_pos_;
  bool added_;

};

class NodeSetPositionAndDependenciesRecursivelyCommand : public UndoCommand{
public:
  NodeSetPositionAndDependenciesRecursivelyCommand(Node* node, Node* context, const Node::Position& pos) :
    node_(node),
    context_(context),
    pos_(pos)
  {}

  virtual Project* GetRelevantProject() const override
  {
    return node_->project();
  }

protected:
  virtual void prepare() override;

  virtual void redo() override;

  virtual void undo() override;

private:
  void move_recursively(Node *node, const QPointF &diff);

  Node* node_;
  Node* context_;
  Node::Position pos_;
  QVector<UndoCommand*> commands_;

};

class NodeRemovePositionFromContextCommand : public UndoCommand
{
public:
  NodeRemovePositionFromContextCommand(Node *node, Node *context) :
    node_(node),
    context_(context)
  {
  }

  virtual Project * GetRelevantProject() const override
  {
    return node_->project();
  }

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  Node *node_;

  Node *context_;

  Node::Position old_pos_;

  bool contained_;

};

class NodeRemovePositionFromAllContextsCommand : public UndoCommand
{
public:
  NodeRemovePositionFromAllContextsCommand(Node *node) :
    node_(node)
  {
  }

  virtual Project * GetRelevantProject() const override
  {
    return node_->project();
  }

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  Node *node_;

  std::map<Node *, QPointF> contexts_;

};

class NodeArrayInsertCommand : public UndoCommand
{
public:
  NodeArrayInsertCommand(Node* node, const QString& input, int index) :
    node_(node),
    input_(input),
    index_(index)
  {
  }

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override
  {
    node_->InputArrayInsert(input_, index_);
  }

  virtual void undo() override
  {
    node_->InputArrayRemove(input_, index_);
  }

private:
  Node* node_;
  QString input_;
  int index_;

};

class NodeArrayResizeCommand : public UndoCommand
{
public:
  NodeArrayResizeCommand(Node* node, const QString& input, int size) :
    node_(node),
    input_(input),
    size_(size)
  {}

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override
  {
    old_size_ = node_->InputArraySize(input_);

    if (old_size_ > size_) {
      // Decreasing in size, disconnect any extraneous edges
      for (auto it = node_->input_connections().cbegin(); it != node_->input_connections().cend(); it++) {
        const NodeOutput &output = it->first;
        const NodeInput &input = it->second;

        if (input.input() == input_ && input.element() >= size_) {
          removed_connections_.push_back({output, input});
          Node::DisconnectEdge(output, input);
        }
      }
    }

    node_->ArrayResizeInternal(input_, size_);
  }

  virtual void undo() override
  {
    for (auto it=removed_connections_.cbegin(); it!=removed_connections_.cend(); it++) {
      Node::ConnectEdge(it->first, it->second);
    }
    removed_connections_.clear();

    node_->ArrayResizeInternal(input_, old_size_);
  }

private:
  Node* node_;
  QString input_;
  int size_;
  int old_size_;

  Node::Connections removed_connections_;

};

class NodeArrayRemoveCommand : public UndoCommand
{
public:
  NodeArrayRemoveCommand(Node* node, const QString& input, int index) :
    node_(node),
    input_(input),
    index_(index)
  {
  }

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override
  {
    // Save immediate data
    if (node_->IsInputKeyframable(input_)) {
      is_keyframing_ = node_->IsInputKeyframing(input_, index_);
    }
    standard_value_ = node_->GetStandardValue(input_, index_);
    keyframes_ = node_->GetKeyframeTracks(input_, index_);
    node_->GetImmediate(input_, index_)->delete_all_keyframes(&memory_manager_);

    node_->InputArrayRemove(input_, index_);
  }

  virtual void undo() override
  {
    node_->InputArrayInsert(input_, index_);

    // Restore keyframes
    foreach (const NodeKeyframeTrack& track, keyframes_) {
      foreach (NodeKeyframe* key, track) {
        key->setParent(node_);
      }
    }
    node_->SetStandardValue(input_, standard_value_, index_);

    if (node_->IsInputKeyframable(input_)) {
      node_->SetInputIsKeyframing(input_, is_keyframing_, index_);
    }
  }

private:
  Node* node_;
  QString input_;
  int index_;

  value_t standard_value_;
  bool is_keyframing_;
  QVector<NodeKeyframeTrack> keyframes_;
  QObject memory_manager_;

};

/**
 * @brief An undoable command for disconnecting two NodeParams
 *
 * Can be considered a UndoCommand wrapper for NodeParam::DisonnectEdge()/
 */
class NodeEdgeRemoveCommand : public UndoCommand {
public:
  NodeEdgeRemoveCommand(const NodeOutput &output, const NodeInput& input);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  NodeOutput output_;
  NodeInput input_;

};

/**
 * @brief An undoable command for connecting two NodeParams together
 *
 * Can be considered a UndoCommand wrapper for NodeParam::ConnectEdge()/
 */
class NodeEdgeAddCommand : public UndoCommand {
public:
  NodeEdgeAddCommand(const NodeOutput &output, const NodeInput& input, int64_t index = -1);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  NodeOutput output_;
  NodeInput input_;
  int64_t index_;

};

class NodeAddCommand : public UndoCommand {
public:
  NodeAddCommand(Project* graph, Node* node);

  void PushToThread(QThread* thread);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  QObject memory_manager_;

  Project* graph_;
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
    return graph_;
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
  Project* graph_;

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
  NodeRenameCommand(Node* node, const QString& new_name)
  {
    AddNode(node, new_name);
  }

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

  void AddEdge(const NodeOutput &output, const NodeInput &input);

  bool ContainsNode(Node *node, Node *context);

  virtual Project * GetRelevantProject() const override;

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  QVector<Node::ContextPair> nodes_;

  QVector<Node::Connection> edges_;

  struct RemovedNode {
    Node *node;
    Node *context;
    QPointF pos;
    Project *removed_from_graph;
  };

  QVector<RemovedNode> removed_nodes_;

  QObject memory_manager_;

};

class NodeParamSetKeyframingCommand : public UndoCommand
{
public:
  NodeParamSetKeyframingCommand(const NodeInput& input, bool setting);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  NodeInput input_;
  bool new_setting_;
  bool old_setting_;

};

class NodeParamInsertKeyframeCommand : public UndoCommand
{
public:
  NodeParamInsertKeyframeCommand(Node *node, NodeKeyframe* keyframe);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  Node* input_;

  NodeKeyframe* keyframe_;

  QObject memory_manager_;

};

class NodeParamRemoveKeyframeCommand : public UndoCommand
{
public:
  NodeParamRemoveKeyframeCommand(NodeKeyframe* keyframe);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  Node* input_;

  NodeKeyframe* keyframe_;

  QObject memory_manager_;

};

class NodeParamSetKeyframeTimeCommand : public UndoCommand
{
public:
  NodeParamSetKeyframeTimeCommand(NodeKeyframe* key, const rational& time);
  NodeParamSetKeyframeTimeCommand(NodeKeyframe* key, const rational& new_time, const rational& old_time);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  NodeKeyframe* key_;

  rational old_time_;
  rational new_time_;

};

class NodeParamSetKeyframeValueCommand : public UndoCommand
{
public:
  NodeParamSetKeyframeValueCommand(NodeKeyframe* key, const value_t::component_t &value);
  NodeParamSetKeyframeValueCommand(NodeKeyframe* key, const value_t::component_t &new_value, const value_t::component_t &old_value);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  NodeKeyframe* key_;

  value_t::component_t old_value_;
  value_t::component_t new_value_;

};

class NodeParamSetStandardValueCommand : public UndoCommand
{
public:
  NodeParamSetStandardValueCommand(const NodeKeyframeTrackReference& input, const value_t::component_t& value);
  NodeParamSetStandardValueCommand(const NodeKeyframeTrackReference& input, const value_t::component_t& new_value, const value_t::component_t& old_value);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  NodeKeyframeTrackReference ref_;

  value_t::component_t old_value_;
  value_t::component_t new_value_;

};

class NodeParamSetSplitStandardValueCommand : public UndoCommand
{
public:
  NodeParamSetSplitStandardValueCommand(const NodeInput& input, const value_t& new_value, const value_t& old_value) :
    ref_(input),
    old_value_(old_value),
    new_value_(new_value)
  {}

  NodeParamSetSplitStandardValueCommand(const NodeInput& input, const value_t& value) :
    NodeParamSetSplitStandardValueCommand(input, value, input.node()->GetStandardValue(input.input()))
  {}

  virtual Project* GetRelevantProject() const override
  {
    return ref_.node()->project();
  }

protected:
  virtual void redo() override
  {
    ref_.node()->SetStandardValue(ref_.input(), new_value_, ref_.element());
  }

  virtual void undo() override
  {
    ref_.node()->SetStandardValue(ref_.input(), old_value_, ref_.element());
  }

private:
  NodeInput ref_;

  value_t old_value_;
  value_t new_value_;

};

class NodeParamArrayAppendCommand : public UndoCommand
{
public:
  NodeParamArrayAppendCommand(Node* node, const QString& input);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  Node* node_;

  QString input_;

};

class NodeSetValueHintCommand : public UndoCommand
{
public:
  NodeSetValueHintCommand(const NodeInput &input, const Node::ValueHint &hint) :
    input_(input),
    new_hint_(hint)
  {
  }

  NodeSetValueHintCommand(Node *node, const QString &input, int element, const Node::ValueHint &hint) :
    NodeSetValueHintCommand(NodeInput(node, input, element), hint)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return input_.node()->project();
  }

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  NodeInput input_;

  Node::ValueHint new_hint_;
  Node::ValueHint old_hint_;

};

class NodeImmediateRemoveAllKeyframesCommand : public UndoCommand
{
public:
  NodeImmediateRemoveAllKeyframesCommand(NodeInputImmediate *immediate) :
    immediate_(immediate)
  {}

  virtual Project* GetRelevantProject() const override { return nullptr; }

protected:
  virtual void prepare() override;

  virtual void redo() override;

  virtual void undo() override;

private:
  NodeInputImmediate *immediate_;

  QObject memory_manager_;

  QVector<NodeKeyframe*> keys_;

};

}

#endif // NODEUNDO_H
