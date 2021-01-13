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

namespace olive {

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

void NodeGraph::childEvent(QChildEvent *event)
{
  Item::childEvent(event);

  Node* node = dynamic_cast<Node*>(event->child());

  if (node) {
    if (event->type() == QEvent::ChildAdded) {

      node_children_.append(node);

      // Connect to each input
      foreach (NodeInput* input, node->parameters()) {
        connect(input, &NodeInput::InputConnected, this, &NodeGraph::SignalInputConnected);
        connect(input, &NodeInput::InputDisconnected, this, &NodeGraph::SignalInputDisconnected);
        connect(input, &NodeInput::ValueChanged, this, &NodeGraph::SignalValueChanged);
      }

      emit NodeAdded(node);

    } else if (event->type() == QEvent::ChildRemoved) {

      node_children_.removeOne(node);

      // Disconnect from inputs
      foreach (NodeInput* input, node->parameters()) {
        disconnect(input, &NodeInput::InputConnected, this, &NodeGraph::SignalInputConnected);
        disconnect(input, &NodeInput::InputDisconnected, this, &NodeGraph::SignalInputDisconnected);
        disconnect(input, &NodeInput::ValueChanged, this, &NodeGraph::SignalValueChanged);
      }

      emit NodeRemoved(node);

    }
  }
}

void NodeGraph::SignalInputConnected(Node *output, int element)
{
  emit InputConnected(output, static_cast<NodeInput*>(sender()), element);
}

void NodeGraph::SignalInputDisconnected(Node *output, int element)
{
  emit InputDisconnected(output, static_cast<NodeInput*>(sender()), element);
}

void NodeGraph::SignalValueChanged(const TimeRange &range, int element)
{
  Q_UNUSED(range)

  emit ValueChanged(static_cast<NodeInput*>(sender()), element);
}

}
