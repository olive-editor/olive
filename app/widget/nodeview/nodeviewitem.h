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

#ifndef NODEVIEWITEM_H
#define NODEVIEWITEM_H

#include <QFontMetrics>
#include <QGraphicsRectItem>
#include <QLinearGradient>
#include <QWidget>

#include "node/node.h"
#include "nodeviewcommon.h"

namespace olive {

class NodeViewEdge;

/**
 * @brief A visual widget representation of a Node object to be used in a NodeView
 *
 * This widget can be collapsed or expanded to show/hide the node's various parameters.
 *
 * To retrieve the NodeViewItem for a certain Node, use NodeView::NodeToUIObject().
 */
class NodeViewItem : public QGraphicsRectItem
{
public:
  NodeViewItem(QGraphicsItem* parent = nullptr);

  QPointF GetNodePosition() const;
  void SetNodePosition(const QPointF& pos);

  /**
   * @brief Set the Node to correspond to this widget
   */
  void SetNode(Node* n);

  /**
   * @brief Get currently attached node
   */
  Node* GetNode() const
  {
    return node_;
  }

  /**
   * @brief Get expanded state
   */
  bool IsExpanded() const
  {
    return expanded_;
  }

  /**
   * @brief Set expanded state
   */
  void SetExpanded(bool e, bool hide_titlebar = false);
  void ToggleExpanded();

  /**
   * @brief Returns GLOBAL point that edges should connect to for any NodeParam member of this object
   */
  QPointF GetInputPoint(const QString& input, int element, const QPointF &source_pos) const;

  QPointF GetOutputPoint(const QString &output) const;

  /**
   * @brief Sets the direction nodes are flowing
   */
  void SetFlowDirection(NodeViewCommon::FlowDirection dir);

  static int DefaultTextPadding();

  static int DefaultItemHeight();

  static int DefaultItemWidth();

  static int DefaultItemBorder();

  qreal DefaultItemHorizontalPadding() const;

  qreal DefaultItemVerticalPadding() const;

  void AddEdge(NodeViewEdge* edge);
  void RemoveEdge(NodeViewEdge* edge);

  int GetIndexAt(QPointF pt) const;

  NodeInput GetInputAtIndex(int index) const
  {
    return NodeInput(node_, node_inputs_.at(index));
  }

  void SetHighlightedIndex(int index);

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
  virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

  virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) override;

private:
  void ReadjustAllEdges();

  /**
   * @brief Returns local rect of a NodeInput in array node_inputs_[index]
   */
  QRectF GetInputRect(int index) const;

  /**
   * @brief Returns local point that edges should connect to for a NodeInput in array node_inputs_[index]
   */
  QPointF GetInputPointInternal(int index, const QPointF &source_pos) const;

  /**
   * @brief Reference to attached Node
   */
  Node* node_;

  /**
   * @brief Cached list of node inputs
   */
  QVector<QString> node_inputs_;

  /**
   * @brief Rectangle of the Node's title bar (equal to rect() when collapsed)
   */
  QRectF title_bar_rect_;

  /// Sizing variables to use when drawing
  int node_border_width_;

  /**
   * @brief Expanded state
   */
  bool expanded_;

  bool hide_titlebar_;

  int highlighted_index_;

  NodeViewCommon::FlowDirection flow_dir_;

  QVector<NodeViewEdge*> edges_;

};

}

#endif // NODEVIEWITEM_H
