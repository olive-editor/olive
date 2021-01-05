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
      emit NodeAdded(node);

    } else if (event->type() == QEvent::ChildRemoved) {

      node_children_.removeOne(node);
      emit NodeRemoved(node);

    }
  }
}

}
