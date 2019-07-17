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

#ifndef NODEEDGEITEM_H
#define NODEEDGEITEM_H

#include <QGraphicsLineItem>

#include "node/edge.h"

/**
 * @brief A graphical representation of a NodeEdge to be used in NodeView
 *
 * A fairly simple line widget use to visualize a connection between two node parameters (a NodeEdge).
 */
class NodeViewEdge : public QGraphicsLineItem
{
public:
  NodeViewEdge(QGraphicsItem* parent = nullptr);

  /**
   * @brief Set the edge that this item corresponds to
   *
   * This can be changed at any time (but under most circumstances won't be). Calling this will automatically call
   * Adjust() to move this item into the correct position.
   */
  void SetEdge(NodeEdgePtr edge);
  NodeEdgePtr edge();

  /**
   * @brief Moves/updates this line to visually connect between the two corresponding NodeViewItems
   *
   * Using the attached edge (see SetEdge()), this function retrieves the NodeViewItems representing the two nodes
   * that this edge connects. It uses their positions to determine where the line should visually connect and sets
   * it accordingly.
   *
   * This should be set any time the NodeEdge changes (see SetEdge()), and any time the nodes move in the NodeGraph
   * (see NodeView::ItemsChanged()). This will keep the nodes visually connected at all times.
   */
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

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
  NodeEdgePtr edge_;

  int edge_width_;

  bool connected_;
};

#endif // NODEEDGEITEM_H
