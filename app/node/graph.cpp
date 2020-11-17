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

#include "graph.h"

OLIVE_NAMESPACE_ENTER

NodeGraph::NodeGraph() :
  operation_stack_(0)
{
}

void NodeGraph::Clear()
{
  foreach (Node* node, node_children_) {
    delete node;
  }
  node_children_.clear();
}

void NodeGraph::AddNode(Node *node)
{
  if (ContainsNode(node)) {
    return;
  }

  node->setParent(this);

  connect(node, &Node::EdgeAdded, this, &NodeGraph::SignalEdgeAdded);
  connect(node, &Node::EdgeRemoved, this, &NodeGraph::SignalEdgeRemoved);

  node_children_.append(node);

  emit NodeAdded(node);
}

void NodeGraph::BeginOperation()
{
  operation_stack_++;
}

void NodeGraph::EndOperation()
{
  operation_stack_--;

  if (!operation_stack_) {
    // Signal everything that we cached during the operation

    // First, signal the removed edges
    foreach (NodeEdgePtr e, cached_removed_edges_) {
      emit EdgeRemoved(e);
    }
    cached_removed_edges_.clear();

    // Next, signal the removed nodes
    foreach (Node* n, cached_removed_nodes_) {
      emit NodeRemoved(n);
    }
    cached_removed_nodes_.clear();

    // Next, signal the added nodes
    foreach (Node* n, cached_added_nodes_) {
      emit NodeAdded(n);
    }
    cached_added_nodes_.clear();

    // Finally, signal the added edges
    foreach (NodeEdgePtr e, cached_added_edges_) {
      emit EdgeAdded(e);
    }
    cached_added_edges_.clear();
  }
}

void NodeGraph::SignalNodeAdded(Node* node)
{
  if (!operation_stack_) {
    emit NodeAdded(node);
  } else if (!cached_removed_nodes_.removeOne(node)) {
    // If we already removed this node during the operation (appending a signal to
    // cached_removed_nodes_), we just remove that instead of appending a new signal. However if we
    // didn't (removeOne returning false), only then do we append an add signal
    cached_added_nodes_.append(node);
  }
}

void NodeGraph::SignalNodeRemoved(Node *node)
{
  if (!operation_stack_) {
    emit NodeRemoved(node);
  } else if (!cached_added_nodes_.removeOne(node)) {
    // See SignalNodeAdded() for explanation of this
    cached_removed_nodes_.append(node);
  }
}

void NodeGraph::SignalEdgeAdded(NodeEdgePtr edge)
{
  if (!operation_stack_) {
    emit EdgeAdded(edge);
  } else if (!cached_removed_edges_.removeOne(edge)) {
    // See SignalNodeAdded() for explanation of this
    cached_added_edges_.append(edge);
  }
}

void NodeGraph::SignalEdgeRemoved(NodeEdgePtr edge)
{
  if (!operation_stack_) {
    emit EdgeRemoved(edge);
  } else if (!cached_added_edges_.removeOne(edge)) {
    // See SignalNodeAdded() for explanation of this
    cached_removed_edges_.append(edge);
  }
}

void NodeGraph::TakeNode(Node *node, QObject* new_parent)
{
  if (!ContainsNode(node)) {
    return;
  }

  if (!node->CanBeDeleted()) {
    qWarning() << "Tried to delete a Node that's been flagged as not deletable";
    return;
  }

  node->DisconnectAll();

  disconnect(node, &Node::EdgeAdded, this, &NodeGraph::EdgeAdded);
  disconnect(node, &Node::EdgeRemoved, this, &NodeGraph::EdgeRemoved);

  node->setParent(new_parent);

  node_children_.removeAll(node);

  emit NodeRemoved(node);
}

const QList<Node *> &NodeGraph::nodes() const
{
  return node_children_;
}

bool NodeGraph::ContainsNode(Node *n) const
{
  return (n->parent() == this);
}

OLIVE_NAMESPACE_EXIT
