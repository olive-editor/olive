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

#ifndef NODEEDGEITEM_H
#define NODEEDGEITEM_H

#include <QGraphicsPathItem>
#include <QPalette>

#include "nodeviewcommon.h"
#include "node/node.h"

namespace olive {

class NodeViewItem;

/**
 * @brief A graphical representation of a NodeEdge to be used in NodeView
 *
 * A fairly simple line widget use to visualize a connection between two node parameters (a NodeEdge).
 */
class NodeViewEdge : public QGraphicsPathItem
{
public:
  NodeViewEdge(Node *output, const NodeInput& input,
               NodeViewItem* from_item, NodeViewItem* to_item,
               QGraphicsItem* parent = nullptr);

  NodeViewEdge(QGraphicsItem* parent = nullptr);

  virtual ~NodeViewEdge() override;

  Node *output() const
  {
    return output_;
  }

  const NodeInput& input() const
  {
    return input_;
  }

  int element() const
  {
    return element_;
  }

  NodeViewItem* from_item() const
  {
    return from_item_;
  }

  NodeViewItem* to_item() const
  {
    return to_item_;
  }

  void set_from_item(NodeViewItem *i);

  void set_to_item(NodeViewItem *i);

  void Adjust();

  /**
   * @brief Set the connected state of this line
   *
   * When the edge is not connected, it visually depicts this by coloring the line grey. When an edge is connected or
   * a potential connection is valid, the line is colored white. This function sets whether the line should be grey
   * (false) or white (true).
   *
   * Using SetEdge() automatically sets this to true. Under most circumstances this should be left alone, and only
   * be set when an edge is being created/dragged.
   */
  void SetConnected(bool c);

  bool IsConnected() const
  {
    return connected_;
  }

  /**
   * @brief Set highlighted state
   *
   * Changes color of edge.
   */
  void SetHighlighted(bool e);

  /**
   * @brief Set points to create curve from
   */
  void SetPoints(const QPointF& start, const QPointF& end);

  /**
   * @brief Set whether edges should be drawn as curved or as straight lines
   */
  void SetCurved(bool e);

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
  void Init();

  void UpdateCurve();

  Node *output_;

  NodeInput input_;

  int element_;

  NodeViewItem* from_item_;

  NodeViewItem* to_item_;

  int edge_width_;

  bool connected_;

  bool highlighted_;

  bool curved_;

  QPointF cached_start_;
  QPointF cached_end_;

};

}

#endif // NODEEDGEITEM_H
