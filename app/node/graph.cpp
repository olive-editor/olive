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

#include "common/qobjectlistcast.h"

NodeGraph::NodeGraph()
{

}

void NodeGraph::AddNode(Node *node)
{
  if (ContainsNode(node)) {
    return;
  }

  node->setParent(this);

  connect(node, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SIGNAL(EdgeAdded(NodeEdgePtr)));
  connect(node, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SIGNAL(EdgeRemoved(NodeEdgePtr)));

  emit NodeAdded(node);
}

void NodeGraph::AddNodeWithDependencies(Node *node)
{
  // Add node and its connected nodes to graph
  AddNode(node);

  // Add all of Block's dependencies
  QList<Node*> node_dependencies = node->GetDependencies();
  foreach (Node* dep, node_dependencies) {
    AddNode(dep);
  }
}

void NodeGraph::TakeNode(Node *node, QObject* new_parent)
{
  if (!ContainsNode(node)) {
    return;
  }

  node->setParent(new_parent);

  disconnect(node, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SIGNAL(EdgeAdded(NodeEdgePtr)));
  disconnect(node, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SIGNAL(EdgeRemoved(NodeEdgePtr)));

  emit NodeRemoved(node);
}

QList<Node *> NodeGraph::TakeNodeWithItsDependencies(Node *node, QObject *new_parent)
{
  if (!ContainsNode(node)) {
    return QList<Node*>();
  }

  QList<Node*> deps = node->GetExclusiveDependencies();

  foreach (Node* d, deps) {
    TakeNode(d, new_parent);
  }

  return deps;
}

QList<Node *> NodeGraph::nodes()
{
  return static_qobjectlist_cast<Node>(children());
}

bool NodeGraph::ContainsNode(Node *n)
{
  return (n->parent() == this);
}

void NodeGraph::Release()
{
  QList<Node*> all_nodes = nodes();

  foreach (Node* n, all_nodes) {
    n->Release();
  }
}
