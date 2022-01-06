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

#include "nodeviewundo.h"

#include "node/project/sequence/sequence.h"

namespace olive {

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

NodeAddCommand::NodeAddCommand(NodeGraph *graph, Node *node) :
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
  return dynamic_cast<Project*>(graph_);
}

NodeCopyInputsCommand::NodeCopyInputsCommand(const Node *src, Node *dest, bool include_connections) :
  src_(src),
  dest_(dest),
  include_connections_(include_connections)
{
}

void NodeCopyInputsCommand::redo()
{
  Node::CopyInputs(src_, dest_, include_connections_);
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
  foreach (const NodePair &pair, nodes_) {
    if (pair.first == node && pair.second == context) {
      return;
    }
  }

  nodes_.append(NodePair({node, context}));

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

Project *NodeViewDeleteCommand::GetRelevantProject() const
{
  if (!nodes_.isEmpty()) {
    return nodes_.first().first->project();
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

  foreach (const NodePair &pair, nodes_) {
    RemovedNode rn;

    rn.node = pair.first;
    rn.context = pair.second;
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

}
