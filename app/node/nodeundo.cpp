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

#include "nodeundo.h"

namespace olive {

void NodeSetPositionCommand::redo()
{
  added_ = !context_->ContextContainsNode(node_);

  if (!added_) {
    old_pos_ = context_->GetNodePositionDataInContext(node_);
  }

  context_->SetNodePositionInContext(node_, pos_);
}

void NodeSetPositionCommand::undo()
{
  if (added_) {
    context_->RemoveNodeFromContext(node_);
  } else {
    context_->SetNodePositionInContext(node_, old_pos_);
  }
}

void NodeRemovePositionFromContextCommand::redo()
{
  contained_ = context_->ContextContainsNode(node_);

  if (contained_) {
    old_pos_ = context_->GetNodePositionDataInContext(node_);
    context_->RemoveNodeFromContext(node_);
  }
}

void NodeRemovePositionFromContextCommand::undo()
{
  if (contained_) {
    context_->SetNodePositionInContext(node_, old_pos_);
  }
}

void NodeRemovePositionFromAllContextsCommand::redo()
{
  Project *graph = node_->parent();

  foreach (Node* context, graph->nodes()) {
    if (context->ContextContainsNode(node_)) {
      contexts_.insert({context, context->GetNodePositionInContext(node_)});
      context->RemoveNodeFromContext(node_);
    }
  }
}

void NodeRemovePositionFromAllContextsCommand::undo()
{
  for (auto it = contexts_.crbegin(); it != contexts_.crend(); it++) {
    it->first->SetNodePositionInContext(node_, it->second);
  }

  contexts_.clear();
}

void NodeSetPositionAndDependenciesRecursivelyCommand::prepare()
{
  move_recursively(node_, pos_.position - context_->GetNodePositionDataInContext(node_).position);
}

void NodeSetPositionAndDependenciesRecursivelyCommand::redo()
{
  for (auto it=commands_.cbegin(); it!=commands_.cend(); it++) {
    (*it)->redo_now();
  }
}

void NodeSetPositionAndDependenciesRecursivelyCommand::undo()
{
  for (auto it=commands_.crbegin(); it!=commands_.crend(); it++) {
    (*it)->undo_now();
  }
}

void NodeSetPositionAndDependenciesRecursivelyCommand::move_recursively(Node *node, const QPointF &diff)
{
  Node::Position pos = context_->GetNodePositionDataInContext(node);
  pos += diff;
  commands_.append(new NodeSetPositionCommand(node_, context_, pos));

  for (auto it=node->input_connections().cbegin(); it!=node->input_connections().cend(); it++) {
    Node *output = it->second;
    if (context_->ContextContainsNode(output)) {
      move_recursively(output, diff);
    }
  }
}

NodeEdgeAddCommand::NodeEdgeAddCommand(Node *output, const NodeInput &input) :
  output_(output),
  input_(input),
  remove_command_(nullptr)
{
}

NodeEdgeAddCommand::~NodeEdgeAddCommand()
{
  delete remove_command_;
}

void NodeEdgeAddCommand::redo()
{
  if (input_.IsConnected()) {
    if (!remove_command_) {
      remove_command_ = new NodeEdgeRemoveCommand(input_.GetConnectedOutput(), input_);
    }

    remove_command_->redo_now();
  }

  Node::ConnectEdge(output_, input_);
}

void NodeEdgeAddCommand::undo()
{
  Node::DisconnectEdge(output_, input_);

  if (remove_command_) {
    remove_command_->undo_now();
  }
}

Project *NodeEdgeAddCommand::GetRelevantProject() const
{
  return output_->project();
}

NodeEdgeRemoveCommand::NodeEdgeRemoveCommand(Node *output, const NodeInput &input) :
  output_(output),
  input_(input)
{
}

void NodeEdgeRemoveCommand::redo()
{
  Node::DisconnectEdge(output_, input_);
}

void NodeEdgeRemoveCommand::undo()
{
  Node::ConnectEdge(output_, input_);
}

Project *NodeEdgeRemoveCommand::GetRelevantProject() const
{
  return output_->project();
}

NodeAddCommand::NodeAddCommand(Project *graph, Node *node) :
  graph_(graph),
  node_(node)
{
  // Ensures that when this command is destroyed, if redo() is never called again, the node will be destroyed too
  node_->setParent(&memory_manager_);
}

void NodeAddCommand::PushToThread(QThread *thread)
{
  memory_manager_.moveToThread(thread);
}

void NodeAddCommand::redo()
{
  node_->setParent(graph_);
}

void NodeAddCommand::undo()
{
  node_->setParent(&memory_manager_);
}

Project *NodeAddCommand::GetRelevantProject() const
{
  return graph_;
}

void NodeRemoveAndDisconnectCommand::prepare()
{
  command_ = new MultiUndoCommand();

  // If this is a block, remove all links
  if (node_->HasLinks()) {
    command_->add_child(new NodeUnlinkAllCommand(node_));
  }

  // Disconnect everything
  for (auto it=node_->input_connections().cbegin(); it!=node_->input_connections().cend(); it++) {
    command_->add_child(new NodeEdgeRemoveCommand(it->second, it->first));
  }

  for (const Node::OutputConnection& conn : node_->output_connections()) {
    command_->add_child(new NodeEdgeRemoveCommand(conn.first, conn.second));
  }

  command_->add_child(new NodeRemovePositionFromAllContextsCommand(node_));
}

void NodeRenameCommand::AddNode(Node *node, const QString &new_name)
{
  nodes_.append(node);
  new_labels_.append(new_name);
  old_labels_.append(node->GetLabel());
}

void NodeRenameCommand::redo()
{
  for (int i=0; i<nodes_.size(); i++) {
    nodes_.at(i)->SetLabel(new_labels_.at(i));
  }
}

void NodeRenameCommand::undo()
{
  for (int i=0; i<nodes_.size(); i++) {
    nodes_.at(i)->SetLabel(old_labels_.at(i));
  }
}

Project *NodeRenameCommand::GetRelevantProject() const
{
  return nodes_.isEmpty() ? nullptr : nodes_.first()->project();
}

NodeOverrideColorCommand::NodeOverrideColorCommand(Node *node, int index) :
  node_(node),
  new_index_(index)
{
}

Project *NodeOverrideColorCommand::GetRelevantProject() const
{
  return node_->project();
}

void NodeOverrideColorCommand::redo()
{
  old_index_ = node_->GetOverrideColor();
  node_->SetOverrideColor(new_index_);
}

void NodeOverrideColorCommand::undo()
{
  node_->SetOverrideColor(old_index_);
}

NodeViewDeleteCommand::NodeViewDeleteCommand()
{
}

void NodeViewDeleteCommand::AddNode(Node *node, Node *context)
{
  if (ContainsNode(node, context)) {
    return;
  }

  Node::ContextPair p = {node, context};
  nodes_.append(p);

  for (auto it=node->input_connections().cbegin(); it!=node->input_connections().cend(); it++) {
    if (context->ContextContainsNode(it->second)) {
      AddEdge(it->second, it->first);
    }
  }

  for (auto it=node->output_connections().cbegin(); it!=node->output_connections().cend(); it++) {
    if (context->ContextContainsNode(it->second.node())) {
      AddEdge(it->first, it->second);
    }
  }
}

void NodeViewDeleteCommand::AddEdge(Node *output, const NodeInput &input)
{
  foreach (const Node::OutputConnection &edge, edges_) {
    if (edge.first == output && edge.second == input) {
      return;
    }
  }

  edges_.append({output, input});
}

bool NodeViewDeleteCommand::ContainsNode(Node *node, Node *context)
{
  foreach (const Node::ContextPair &pair, nodes_) {
    if (pair.node == node && pair.context == context) {
      return true;
    }
  }

  return false;
}

Project *NodeViewDeleteCommand::GetRelevantProject() const
{
  if (!nodes_.isEmpty()) {
    return nodes_.first().node->project();
  }

  if (!edges_.isEmpty()) {
    return edges_.first().first->project();
  }

  return nullptr;
}

void NodeViewDeleteCommand::redo()
{
  foreach (const Node::OutputConnection &edge, edges_) {
    Node::DisconnectEdge(edge.first, edge.second);
  }

  foreach (const Node::ContextPair &pair, nodes_) {
    RemovedNode rn;

    rn.node = pair.node;
    rn.context = pair.context;
    rn.pos = rn.context->GetNodePositionInContext(rn.node);

    rn.context->RemoveNodeFromContext(rn.node);

    // If node is no longer in any contexts and is not connected to anything, remove it
    if (rn.node->parent()->GetNumberOfContextsNodeIsIn(rn.node, true) == 0
        && rn.node->input_connections().empty()
        && rn.node->output_connections().empty()) {
      rn.removed_from_graph = rn.node->parent();
      rn.node->setParent(&memory_manager_);
    } else {
      rn.removed_from_graph = nullptr;
    }

    removed_nodes_.append(rn);
  }
}

void NodeViewDeleteCommand::undo()
{
  for (auto rn=removed_nodes_.crbegin(); rn!=removed_nodes_.crend(); rn++) {
    if (rn->removed_from_graph) {
      rn->node->setParent(rn->removed_from_graph);
    }

    rn->context->SetNodePositionInContext(rn->node, rn->pos);
  }
  removed_nodes_.clear();

  for (auto edge=edges_.crbegin(); edge!=edges_.crend(); edge++) {
    Node::ConnectEdge(edge->first, edge->second);
  }
}

NodeParamSetKeyframingCommand::NodeParamSetKeyframingCommand(const NodeInput &input, bool setting) :
  input_(input),
  new_setting_(setting)
{
}

Project *NodeParamSetKeyframingCommand::GetRelevantProject() const
{
  return input_.node()->project();
}

void NodeParamSetKeyframingCommand::redo()
{
  old_setting_ = input_.IsKeyframing();
  input_.node()->SetInputIsKeyframing(input_, new_setting_);
}

void NodeParamSetKeyframingCommand::undo()
{
  input_.node()->SetInputIsKeyframing(input_, old_setting_);
}

NodeParamSetKeyframeValueCommand::NodeParamSetKeyframeValueCommand(NodeKeyframe* key, const QVariant& value) :
  key_(key),
  old_value_(key_->value()),
  new_value_(value)
{
}

NodeParamSetKeyframeValueCommand::NodeParamSetKeyframeValueCommand(NodeKeyframe* key, const QVariant &new_value, const QVariant &old_value) :
  key_(key),
  old_value_(old_value),
  new_value_(new_value)
{

}

Project *NodeParamSetKeyframeValueCommand::GetRelevantProject() const
{
  return key_->parent()->project();
}

void NodeParamSetKeyframeValueCommand::redo()
{
  key_->set_value(new_value_);
}

void NodeParamSetKeyframeValueCommand::undo()
{
  key_->set_value(old_value_);
}

NodeParamInsertKeyframeCommand::NodeParamInsertKeyframeCommand(Node* node, NodeKeyframe* keyframe) :
  input_(node),
  keyframe_(keyframe)
{
  // Take ownership of the keyframe
  undo();
}

Project *NodeParamInsertKeyframeCommand::GetRelevantProject() const
{
  return input_->project();
}

void NodeParamInsertKeyframeCommand::redo()
{
  keyframe_->setParent(input_);
}

void NodeParamInsertKeyframeCommand::undo()
{
  keyframe_->setParent(&memory_manager_);
}

NodeParamRemoveKeyframeCommand::NodeParamRemoveKeyframeCommand(NodeKeyframe* keyframe) :
  input_(keyframe->parent()),
  keyframe_(keyframe)
{
}

Project *NodeParamRemoveKeyframeCommand::GetRelevantProject() const
{
  return input_->project();
}

void NodeParamRemoveKeyframeCommand::redo()
{
  // Removes from input
  keyframe_->setParent(&memory_manager_);
}

void NodeParamRemoveKeyframeCommand::undo()
{
  keyframe_->setParent(input_);
}

NodeParamSetKeyframeTimeCommand::NodeParamSetKeyframeTimeCommand(NodeKeyframe* key, const rational &time) :
  key_(key),
  old_time_(key->time()),
  new_time_(time)
{
}

NodeParamSetKeyframeTimeCommand::NodeParamSetKeyframeTimeCommand(NodeKeyframe* key, const rational &new_time, const rational &old_time) :
  key_(key),
  old_time_(old_time),
  new_time_(new_time)
{
}

Project *NodeParamSetKeyframeTimeCommand::GetRelevantProject() const
{
  return key_->parent()->project();
}

void NodeParamSetKeyframeTimeCommand::redo()
{
  key_->set_time(new_time_);
}

void NodeParamSetKeyframeTimeCommand::undo()
{
  key_->set_time(old_time_);
}

NodeParamSetStandardValueCommand::NodeParamSetStandardValueCommand(const NodeKeyframeTrackReference& input, const QVariant &value) :
  ref_(input),
  old_value_(ref_.input().node()->GetStandardValue(ref_.input())),
  new_value_(value)
{
}

NodeParamSetStandardValueCommand::NodeParamSetStandardValueCommand(const NodeKeyframeTrackReference& input, const QVariant &new_value, const QVariant &old_value) :
  ref_(input),
  old_value_(old_value),
  new_value_(new_value)
{
}

Project *NodeParamSetStandardValueCommand::GetRelevantProject() const
{
  return ref_.input().node()->project();
}

void NodeParamSetStandardValueCommand::redo()
{
  ref_.input().node()->SetSplitStandardValueOnTrack(ref_, new_value_);
}

void NodeParamSetStandardValueCommand::undo()
{
  ref_.input().node()->SetSplitStandardValueOnTrack(ref_, old_value_);
}

NodeParamArrayAppendCommand::NodeParamArrayAppendCommand(Node *node, const QString &input) :
  node_(node),
  input_(input)
{
}

Project *NodeParamArrayAppendCommand::GetRelevantProject() const
{
  return node_->project();
}

void NodeParamArrayAppendCommand::redo()
{
  node_->InputArrayAppend(input_);
}

void NodeParamArrayAppendCommand::undo()
{
  node_->InputArrayRemoveLast(input_);
}

void NodeSetValueHintCommand::redo()
{
  old_hint_ = input_.node()->GetValueHintForInput(input_.input(), input_.element());
  input_.node()->SetValueHintForInput(input_.input(), new_hint_, input_.element());
}

void NodeSetValueHintCommand::undo()
{
  input_.node()->SetValueHintForInput(input_.input(), old_hint_, input_.element());
}

Project *NodeArrayInsertCommand::GetRelevantProject() const
{
  return node_->project();
}

Project *NodeArrayRemoveCommand::GetRelevantProject() const
{
  return node_->project();
}

Project *NodeArrayResizeCommand::GetRelevantProject() const
{
  return node_->project();
}

void NodeImmediateRemoveAllKeyframesCommand::prepare()
{
  for (const NodeKeyframeTrack& track : immediate_->keyframe_tracks()) {
    keys_.append(track);
  }
}

void NodeImmediateRemoveAllKeyframesCommand::redo()
{
  for (auto it=keys_.cbegin(); it!=keys_.cend(); it++) {
    (*it)->setParent(&memory_manager_);
  }
}

void NodeImmediateRemoveAllKeyframesCommand::undo()
{
  for (auto it=keys_.crbegin(); it!=keys_.crend(); it++) {
    (*it)->setParent(&memory_manager_);
  }
}

}
