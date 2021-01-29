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

NodeEdgeAddCommand::NodeEdgeAddCommand(Node *output, NodeInput *input, int element) :
  output_(output),
  input_(input),
  element_(element),
  remove_command_(nullptr)
{
}

NodeEdgeAddCommand::~NodeEdgeAddCommand()
{
  delete remove_command_;
}

void NodeEdgeAddCommand::redo()
{
  if (input_->IsConnected(element_)) {
    if (!remove_command_) {
      remove_command_ = new NodeEdgeRemoveCommand(input_->GetConnectedNode(element_),
                                                  input_,
                                                  element_);
    }

    remove_command_->redo();
  }

  Node::ConnectEdge(output_, input_, element_);
}

void NodeEdgeAddCommand::undo()
{
  Node::DisconnectEdge(output_, input_, element_);

  if (remove_command_) {
    remove_command_->undo();
  }
}

Project *NodeEdgeAddCommand::GetRelevantProject() const
{
  return output_->parent()->project();
}

NodeEdgeRemoveCommand::NodeEdgeRemoveCommand(Node *output, NodeInput *input, int element) :
  output_(output),
  input_(input),
  element_(element)
{
}

void NodeEdgeRemoveCommand::redo()
{
  Node::DisconnectEdge(output_, input_, element_);
}

void NodeEdgeRemoveCommand::undo()
{
  Node::ConnectEdge(output_, input_, element_);
}

Project *NodeEdgeRemoveCommand::GetRelevantProject() const
{
  return output_->parent()->project();
}

NodeAddCommand::NodeAddCommand(NodeGraph *graph, Node *node) :
  graph_(graph),
  node_(node)
{
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
  return graph_->project();
}

NodeCopyInputsCommand::NodeCopyInputsCommand(Node *src, Node *dest, bool include_connections) :
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
  foreach (const Node::InputConnection& conn, node_->edges()) {
    command_->add_child(new NodeEdgeRemoveCommand(node_, conn.input, conn.element));
  }

  foreach (NodeInput* input, node_->inputs()) {
    for (auto it=input->edges().cbegin(); it!=input->edges().cend(); it++) {
      command_->add_child(new NodeEdgeRemoveCommand(it->second, input, it->first));
    }
  }
}

}
