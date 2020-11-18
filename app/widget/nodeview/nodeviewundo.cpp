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

namespace olive {

NodeEdgeAddCommand::NodeEdgeAddCommand(NodeOutput *output, NodeInput *input, QUndoCommand *parent) :
  UndoCommand(parent),
  output_(output),
  input_(input),
  old_edge_(nullptr),
  done_(false)
{
}

void NodeEdgeAddCommand::redo_internal()
{
  if (done_) {
    return;
  }

  old_edge_ = NodeParam::DisconnectForNewOutput(input_);

  NodeParam::ConnectEdge(output_, input_);

  done_ = true;
}

void NodeEdgeAddCommand::undo_internal()
{
  if (!done_) {
    return;
  }

  NodeParam::DisconnectEdge(output_, input_);

  if (old_edge_ != nullptr) {
    NodeParam::ConnectEdge(old_edge_->output(), old_edge_->input());
  }

  done_ = false;
}

Project *NodeEdgeAddCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(output_->parentNode()->parent())->project();
}

NodeEdgeRemoveCommand::NodeEdgeRemoveCommand(NodeOutput *output, NodeInput *input, QUndoCommand *parent) :
  UndoCommand(parent),
  output_(output),
  input_(input),
  done_(false)
{
}

NodeEdgeRemoveCommand::NodeEdgeRemoveCommand(NodeEdgePtr edge, QUndoCommand *parent) :
  UndoCommand(parent),
  output_(edge->output()),
  input_(edge->input()),
  done_(false)
{
}

void NodeEdgeRemoveCommand::redo_internal()
{
  if (done_) {
    return;
  }

  NodeParam::DisconnectEdge(output_, input_);
  done_ = true;
}

void NodeEdgeRemoveCommand::undo_internal()
{
  if (!done_) {
    return;
  }

  NodeParam::ConnectEdge(output_, input_);
  done_ = false;
}

Project *NodeEdgeRemoveCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(output_->parentNode()->parent())->project();
}

NodeAddCommand::NodeAddCommand(NodeGraph *graph, Node *node, QUndoCommand *parent) :
  UndoCommand(parent),
  graph_(graph),
  node_(node)
{
  // Ensures that when this command is destroyed, if redo() is never called again, the node will be destroyed too
  node_->setParent(&memory_manager_);
}

void NodeAddCommand::redo_internal()
{
  graph_->AddNode(node_);
}

void NodeAddCommand::undo_internal()
{
  graph_->TakeNode(node_, &memory_manager_);
}

Project *NodeAddCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(graph_)->project();
}

NodeRemoveCommand::NodeRemoveCommand(NodeGraph *graph, const QVector<Node *> &nodes, QUndoCommand *parent) :
  UndoCommand(parent),
  graph_(graph),
  nodes_(nodes)
{
}

void NodeRemoveCommand::redo_internal()
{
  // Cache edges for undoing
  foreach (Node* n, nodes_) {
    foreach (NodeParam* param, n->parameters()) {
      foreach (NodeEdgePtr edge, param->edges()) {
        // Ensures the same edge isn't added twice (prevents double connecting when undoing)
        if (!edges_.contains(edge)) {
          edges_.append(edge);
        }
      }
    }
  }

  // Take nodes from graph (TakeNode() will automatically disconnect edges)
  foreach (Node* n, nodes_) {
    // If the node is a block, unlink any linked blocks before removing
    if (n->IsBlock()) {
      Block *b = static_cast<Block *>(n);
      if (b->HasLinks()) {
        BlockUnlinkAllCommand *unlink_command = new BlockUnlinkAllCommand(b);
        unlink_command->redo();
        block_unlink_commands_.append(unlink_command);
      }
    }
    graph_->TakeNode(n, &memory_manager_);
  }
}

void NodeRemoveCommand::undo_internal()
{
  // Re-add nodes to graph
  foreach (Node* n, nodes_) {
    graph_->AddNode(n);
  }

  // Relink any blocks that were unlinked
  foreach(BlockUnlinkAllCommand* command, block_unlink_commands_) {
    command->undo();
    delete command;
  }

  // Re-connect edges
  foreach (NodeEdgePtr edge, edges_) {
    NodeParam::ConnectEdge(edge->output(), edge->input());
  }

  edges_.clear();
  block_unlink_commands_.clear();
}

Project *NodeRemoveCommand::GetRelevantProject() const
{
  return static_cast<Sequence*>(graph_)->project();
}

NodeRemoveWithExclusiveDeps::NodeRemoveWithExclusiveDeps(NodeGraph *graph, Node *node, QUndoCommand *parent) :
  UndoCommand(parent)
{
  QVector<Node*> node_and_its_deps;
  node_and_its_deps.append(node);
  node_and_its_deps.append(node->GetExclusiveDependencies());

  remove_command_ = new NodeRemoveCommand(graph, node_and_its_deps, this);
}

Project *NodeRemoveWithExclusiveDeps::GetRelevantProject() const
{
  return remove_command_->GetRelevantProject();
}

NodeCopyInputsCommand::NodeCopyInputsCommand(Node *src, Node *dest, bool include_connections, QUndoCommand *parent) :
  QUndoCommand(parent),
  src_(src),
  dest_(dest),
  include_connections_(include_connections)
{
}

NodeCopyInputsCommand::NodeCopyInputsCommand(Node *src, Node *dest, QUndoCommand *parent) :
  QUndoCommand(parent),
  src_(src),
  dest_(dest),
  include_connections_(true)
{
}

void NodeCopyInputsCommand::redo()
{
  Node::CopyInputs(src_, dest_, include_connections_);
}

}
