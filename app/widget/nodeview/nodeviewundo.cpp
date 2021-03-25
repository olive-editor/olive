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

#include "nodeviewundo.h"

#include "project/item/sequence/sequence.h"
#include "widget/timelinewidget/timelineundo.h"

namespace olive {

NodeEdgeAddCommand::NodeEdgeAddCommand(const NodeOutput &output, const NodeInput &input) :
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

    remove_command_->redo();
  }

  Node::ConnectEdge(output_, input_);
}

void NodeEdgeAddCommand::undo()
{
  Node::DisconnectEdge(output_, input_);

  if (remove_command_) {
    remove_command_->undo();
  }
}

Project *NodeEdgeAddCommand::GetRelevantProject() const
{
  return output_.node()->project();
}

NodeEdgeRemoveCommand::NodeEdgeRemoveCommand(const NodeOutput &output, const NodeInput &input) :
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
  return output_.node()->project();
}

NodeAddCommand::NodeAddCommand(NodeGraph *graph, Node *node) :
  graph_(graph),
  node_(node)
{
  if (memory_manager_.thread() != node->thread()) {
    memory_manager_.moveToThread(node_->thread());
  }

  // Ensures that when this command is destroyed, if redo() is never called again, the node will be destroyed too
  node_->setParent(&memory_manager_);
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

void NodeRemoveAndDisconnectCommand::prep()
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

}
