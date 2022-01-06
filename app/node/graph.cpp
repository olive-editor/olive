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

#include "graph.h"

#include <QChildEvent>

namespace olive {

#define super QObject

NodeGraph::NodeGraph()
{
}

NodeGraph::~NodeGraph()
{
  Clear();
}

void NodeGraph::Clear()
{
  // By deleting the last nodes first, we assume that nodes that are most important are deleted last
  // (e.g. Project's ColorManager or ProjectSettingsNode.
  while (!node_children_.isEmpty()) {
    delete node_children_.last();
  }
}

int NodeGraph::GetNumberOfContextsNodeIsIn(Node *node, bool except_itself) const
{
  int count = 0;

  foreach (Node *ctx, node_children_) {
    if (ctx->ContextContainsNode(node) && (!except_itself || ctx != node)) {
      count++;
    }
  }

  return count;
}

void NodeGraph::childEvent(QChildEvent *event)
{
  super::childEvent(event);

  Node* node = dynamic_cast<Node*>(event->child());

  if (node) {
    if (event->type() == QEvent::ChildAdded) {

      node_children_.append(node);

      // Connect signals
      connect(node, &Node::InputConnected, this, &NodeGraph::InputConnected, Qt::DirectConnection);
      connect(node, &Node::InputDisconnected, this, &NodeGraph::InputDisconnected, Qt::DirectConnection);
      connect(node, &Node::ValueChanged, this, &NodeGraph::ValueChanged, Qt::DirectConnection);
      connect(node, &Node::InputValueHintChanged, this, &NodeGraph::InputValueHintChanged, Qt::DirectConnection);

      if (NodeGroup *group = dynamic_cast<NodeGroup*>(node)) {
        connect(group, &NodeGroup::InputPassthroughAdded, this, &NodeGraph::GroupAddedInputPassthrough, Qt::DirectConnection);
        connect(group, &NodeGroup::InputPassthroughRemoved, this, &NodeGraph::GroupRemovedInputPassthrough, Qt::DirectConnection);
        connect(group, &NodeGroup::OutputPassthroughChanged, this, &NodeGraph::GroupChangedOutputPassthrough, Qt::DirectConnection);
      }

      emit NodeAdded(node);
      emit node->AddedToGraph(this);

    } else if (event->type() == QEvent::ChildRemoved) {

      node_children_.removeOne(node);

      // Disconnect signals
      disconnect(node, &Node::InputConnected, this, &NodeGraph::InputConnected);
      disconnect(node, &Node::InputDisconnected, this, &NodeGraph::InputDisconnected);
      disconnect(node, &Node::ValueChanged, this, &NodeGraph::ValueChanged);
      disconnect(node, &Node::InputValueHintChanged, this, &NodeGraph::InputValueHintChanged);

      if (NodeGroup *group = dynamic_cast<NodeGroup*>(node)) {
        disconnect(group, &NodeGroup::InputPassthroughAdded, this, &NodeGraph::GroupAddedInputPassthrough);
        disconnect(group, &NodeGroup::InputPassthroughRemoved, this, &NodeGraph::GroupRemovedInputPassthrough);
        disconnect(group, &NodeGroup::OutputPassthroughChanged, this, &NodeGraph::GroupChangedOutputPassthrough);
      }

      emit NodeRemoved(node);
      emit node->RemovedFromGraph(this);

      // Remove from any contexts
      foreach (Node *context, node_children_) {
        context->RemoveNodeFromContext(node);
      }
    }
  }
}

}
