/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

NodeGraph::NodeGraph()
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

  connect(node, &Node::EdgeAdded, this, &NodeGraph::EdgeAdded);
  connect(node, &Node::EdgeRemoved, this, &NodeGraph::EdgeRemoved);

  node_children_.append(node);

  emit NodeAdded(node);
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
