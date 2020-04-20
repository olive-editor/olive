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

#ifndef NODEVIEWITEM_H
#define NODEVIEWITEM_H

#include <QFontMetrics>
#include <QGraphicsRectItem>
#include <QLinearGradient>
#include <QUndoCommand>
#include <QWidget>
#include <QPointer>

#include "node/node.h"
#include "nodeviewedge.h"
#include "nodeviewitemwidgetproxy.h"

OLIVE_NAMESPACE_ENTER

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

  /**
   * @brief Set the Node to correspond to this widget
   */
  void SetNode(Node* n);

  /**
   * @brief Get currently attached node
   */
  Node* GetNode() const;

  /**
   * @brief Get expanded state
   */
  bool IsExpanded() const;

  /**
   * @brief Set expanded state
   */
  void SetExpanded(bool e);
  void ToggleExpanded();

  /**
   * @brief Returns GLOBAL point that edges should connect to for any NodeParam member of this object
   */
  QPointF GetParamPoint(NodeParam* param) const;

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

  virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) override;

private:
  /**
   * @brief Highlight an input (for instance when the user is dragging over it)
   */
  void SetHighlightedIndex(int index);

  /**
   * @brief Returns local rect of a NodeInput in array node_inputs_[index]
   */
  QRectF GetInputRect(int index) const;

  /**
   * @brief Returns local point that edges should connect to for a NodeInput in array node_inputs_[index]
   */
  QPointF GetInputPoint(int index) const;

  /**
   * @brief Reference to attached Node
   */
  Node* node_;

  /**
   * @brief Cached list of node inputs
   */
  QList<NodeInput*> node_inputs_;

  /**
   * @brief A QWidget that can receive CSS properties that NodeViewItem can use
   *
   * \see NodeViewItemWidget
   */
  QPointer<NodeDefaultStyleWidget> css_proxy_;

  /**
   * @brief Rectangle of the Node's title bar (equal to rect() when collapsed)
   */
  QRectF title_bar_rect_;

  /// Edge dragging variables
  NodeViewEdge* dragging_edge_;
  QPointF dragging_edge_start_;
  NodeParam* drag_src_param_;
  NodeParam* drag_dest_param_;
  NodeViewItem* drag_source_;

  NodeViewItem* cached_drop_item_;
  bool cached_drop_item_expanded_;

  /// Sizing variables to use when drawing
  int node_border_width_;

  /**
   * @brief Expanded state
   */
  bool expanded_;

  /**
   * @brief Current click mode
   *
   * \see mousePressEvent()
   */
  bool standard_click_;

  int highlighted_index_;

  /**
   * @brief QUndoCommand for creating and deleting edges by dragging
   *
   * \see mousePressEvent()
   * \see mouseReleaseEvent()
   */
  QUndoCommand* node_edge_change_command_;

};

OLIVE_NAMESPACE_EXIT

#endif // NODEVIEWITEM_H
