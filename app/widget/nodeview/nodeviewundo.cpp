#include "nodeviewundo.h"

NodeEdgeAddCommand::NodeEdgeAddCommand(NodeOutput *output, NodeInput *input, QUndoCommand *parent) :
  QUndoCommand(parent),
  output_(output),
  input_(input),
  old_edge_(nullptr),
  done_(false)
{
}

void NodeEdgeAddCommand::redo()
{
  if (done_) {
    return;
  }

  old_edge_ = NodeParam::DisconnectForNewOutput(input_);

  NodeParam::ConnectEdge(output_, input_);

  done_ = true;
}

void NodeEdgeAddCommand::undo()
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

NodeEdgeRemoveCommand::NodeEdgeRemoveCommand(NodeOutput *output, NodeInput *input, QUndoCommand *parent) :
  QUndoCommand(parent),
  output_(output),
  input_(input),
  done_(false)
{
}

NodeEdgeRemoveCommand::NodeEdgeRemoveCommand(NodeEdgePtr edge, QUndoCommand *parent) :
  QUndoCommand(parent),
  output_(edge->output()),
  input_(edge->input()),
  done_(false)
{
}

void NodeEdgeRemoveCommand::redo()
{
  if (done_) {
    return;
  }

  NodeParam::DisconnectEdge(output_, input_);
  done_ = true;
}

void NodeEdgeRemoveCommand::undo()
{
  if (!done_) {
    return;
  }

  NodeParam::ConnectEdge(output_, input_);
  done_ = false;
}

NodeAddCommand::NodeAddCommand(NodeGraph *graph, Node *node, QUndoCommand *parent) :
  QUndoCommand(parent),
  graph_(graph),
  node_(node)
{
  // Ensures that when this command is destroyed, if redo() is never called again, the node will be destroyed too
  node_->setParent(&memory_manager_);
}

void NodeAddCommand::redo()
{
  graph_->AddNode(node_);
}

void NodeAddCommand::undo()
{
  graph_->TakeNode(node_, &memory_manager_);
}

NodeRemoveCommand::NodeRemoveCommand(NodeGraph *graph, const QList<Node *> &nodes, QUndoCommand *parent) :
  QUndoCommand(parent),
  graph_(graph),
  nodes_(nodes)
{
}

void NodeRemoveCommand::redo()
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
    graph_->TakeNode(n, &memory_manager_);
  }
}

void NodeRemoveCommand::undo()
{
  // Re-add nodes to graph
  foreach (Node* n, nodes_) {
    graph_->AddNode(n);
  }

  // Re-connect edges
  foreach (NodeEdgePtr edge, edges_) {
    NodeParam::ConnectEdge(edge->output(), edge->input());
  }

  edges_.clear();
}

NodeRemoveWithExclusiveDeps::NodeRemoveWithExclusiveDeps(NodeGraph *graph, Node *node, QUndoCommand *parent) :
  QUndoCommand(parent)
{
  QList<Node*> node_and_its_deps;
  node_and_its_deps.append(node);
  node_and_its_deps.append(node->GetExclusiveDependencies());

  remove_command_ = new NodeRemoveCommand(graph, node_and_its_deps, this);
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
