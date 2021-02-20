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

#ifndef NODEGRAPH_H
#define NODEGRAPH_H

#include "node/node.h"
#include "project/item/item.h"

namespace olive {

/**
 * @brief A collection of nodes
 *
 * This doesn't technically need to be a derivative of Item, but since both Item and NodeGraph need
 * to be QObject derivatives, this simplifies Sequence.
 */
class NodeGraph : public QObject
{
  Q_OBJECT
public:
  /**
   * @brief NodeGraph Constructor
   */
  NodeGraph();

  /**
   * @brief Destructively destroys all nodes in the graph
   */
  void Clear();

  /**
   * @brief Retrieve a complete list of the nodes belonging to this graph
   */
  const QVector<Node*>& nodes() const
  {
    return node_children_;
  }

  const QVector<Node*>& default_nodes() const
  {
    return default_nodes_;
  }

signals:
  /**
   * @brief Signal emitted when a Node is added to the graph
   */
  void NodeAdded(Node* node);

  /**
   * @brief Signal emitted when a Node is removed from the graph
   */
  void NodeRemoved(Node* node);

  void InputConnected(const NodeOutput& output, const NodeInput& input);

  void InputDisconnected(const NodeOutput& output, const NodeInput& input);

  void ValueChanged(const NodeInput& input);

protected:
  void AddDefaultNode(Node* n)
  {
    default_nodes_.append(n);
  }

  virtual void childEvent(QChildEvent* event) override;

private:
  QVector<Node*> node_children_;

  QVector<Node*> default_nodes_;

};

}

#endif // NODEGRAPH_H
