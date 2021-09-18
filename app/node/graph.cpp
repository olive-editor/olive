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

qreal NodeGraph::GetNodeContextHeight(Node *context)
{
  const PositionMap &map = position_map_.value(context);

  qreal top = 0, bottom = 0;

  foreach (const QPointF &pt, map) {
    top = qMin(pt.y(), top);
    bottom = qMax(pt.y(), bottom);
  }

  return bottom - top;
}

int NodeGraph::GetNumberOfContextsNodeIsIn(Node *node) const
{
  int count = 0;

  for (auto it=position_map_.cbegin(); it!=position_map_.cend(); it++) {
    if (it.value().contains(node)) {
      count++;
    }
  }

  return count;
}

bool NodeGraph::NodeOutputsToContext(Node *node) const
{
  for (auto it=position_map_.cbegin(); it!=position_map_.cend(); it++) {
    const PositionMap &pm = it.value();
    if (pm.contains(node) && node->OutputsTo(it.key(), true)) {
      return true;
    }
  }

  return false;
}

void NodeGraph::childEvent(QChildEvent *event)
{
  super::childEvent(event);

  Node* node = dynamic_cast<Node*>(event->child());

  if (node) {
    if (event->type() == QEvent::ChildAdded) {

      node_children_.append(node);

      // Connect signals
      connect(node, &Node::InputConnected, this, &NodeGraph::InputConnected);
      connect(node, &Node::InputDisconnected, this, &NodeGraph::InputDisconnected);
      connect(node, &Node::ValueChanged, this, &NodeGraph::ValueChanged);
      connect(node, &Node::InputValueHintChanged, this, &NodeGraph::InputValueHintChanged);

      emit NodeAdded(node);
      emit node->AddedToGraph(this);

    } else if (event->type() == QEvent::ChildRemoved) {

      node_children_.removeOne(node);

      // Disconnect signals
      disconnect(node, &Node::InputConnected, this, &NodeGraph::InputConnected);
      disconnect(node, &Node::InputDisconnected, this, &NodeGraph::InputDisconnected);
      disconnect(node, &Node::ValueChanged, this, &NodeGraph::ValueChanged);
      disconnect(node, &Node::InputValueHintChanged, this, &NodeGraph::InputValueHintChanged);

      emit NodeRemoved(node);
      emit node->RemovedFromGraph(this);

      for (auto it=position_map_.begin(); it!=position_map_.end(); it++) {
        PositionMap &map = it.value();
        for (auto jt=map.begin(); jt!=map.end(); ) {
          if (jt.key() == node) {
            jt = map.erase(jt);
            emit NodePositionRemoved(node, it.key());
          } else {
            jt++;
          }
        }
      }

    }
  }
}

}
