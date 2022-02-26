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

#ifndef NODEVIEWITEM_H
#define NODEVIEWITEM_H

#include <QFontMetrics>
#include <QGraphicsRectItem>
#include <QLinearGradient>
#include <QWidget>

#include "node/node.h"
#include "nodeviewcommon.h"
#include "nodeviewitemconnector.h"

namespace olive {

class NodeViewItem;
class NodeViewEdge;

/**
 * @brief A visual widget representation of a Node object to be used in a NodeView
 *
 * This widget can be collapsed or expanded to show/hide the node's various parameters.
 *
 * To retrieve the NodeViewItem for a certain Node, use NodeView::NodeToUIObject().
 */
class NodeViewItem : public QObject, public QGraphicsRectItem
{
  Q_OBJECT
public:
  NodeViewItem(Node *node, const QString &input, int element, Node *context, QGraphicsItem* parent = nullptr);
  NodeViewItem(Node *node, Node *context, QGraphicsItem* parent = nullptr) :
    NodeViewItem(node, QString(), -1, context, parent)
  {
  }

  virtual ~NodeViewItem() override;

  Node::Position GetNodePositionData() const;
  QPointF GetNodePosition() const;
  void SetNodePosition(const QPointF& pos);
  void SetNodePosition(const Node::Position& pos);

  QVector<NodeViewEdge*> GetAllEdgesRecursively() const;

  /**
   * @brief Get currently attached node
   */
  Node* GetNode() const
  {
    return node_;
  }

  NodeInput GetInput() const
  {
    return NodeInput(node_, input_, element_);
  }

  Node *GetContext() const
  {
    return context_;
  }

  /**
   * @brief Get expanded state
   */
  bool IsExpanded() const
  {
    return expanded_;
  }

  const QVector<NodeViewEdge*> &edges() const
  {
    return edges_;
  }

  /**
   * @brief Set expanded state
   */
  void SetExpanded(bool e, bool hide_titlebar = false);
  void ToggleExpanded();

  QPointF GetInputPoint() const;
  QPointF GetOutputPoint() const;

  /**
   * @brief Sets the direction nodes are flowing
   */
  void SetFlowDirection(NodeViewCommon::FlowDirection dir);

  NodeViewCommon::FlowDirection GetFlowDirection() const
  {
    return flow_dir_;
  }

  static int DefaultTextPadding();

  static int DefaultItemHeight();

  static int DefaultItemWidth();

  static int DefaultItemBorder();

  static QPointF NodeToScreenPoint(QPointF p, NodeViewCommon::FlowDirection direction);
  static QPointF ScreenToNodePoint(QPointF p, NodeViewCommon::FlowDirection direction);

  static qreal DefaultItemHorizontalPadding(NodeViewCommon::FlowDirection dir);
  static qreal DefaultItemVerticalPadding(NodeViewCommon::FlowDirection dir);
  qreal DefaultItemHorizontalPadding() const;
  qreal DefaultItemVerticalPadding() const;

  void AddEdge(NodeViewEdge* edge);
  void RemoveEdge(NodeViewEdge* edge);

  bool IsLabelledAsOutputOfContext() const
  {
    return label_as_output_;
  }

  void SetLabelAsOutput(bool e);

  void SetHighlighted(bool e);

  NodeViewItem *GetItemForInput(NodeInput input);

  bool IsOutputItem() const
  {
    return input_.isEmpty();
  }

  void ReadjustAllEdges();

  void UpdateFlowDirectionOfInputItem(NodeViewItem *child);

  bool CanBeExpanded() const;

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

  virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) override;

private:
  void UpdateContextRect();

  void DrawNodeTitle(QPainter *painter, QString text, const QRectF &rect, Qt::Alignment vertical_align, int icon_full_size);

  int DrawExpandArrow(QPainter *painter);

  /**
   * @brief Internal update function when logical position changes
   */
  void UpdateNodePosition();

  void UpdateInputConnectorPosition();
  void UpdateOutputConnectorPosition();

  bool IsInputValid(const QString &input);

  void SetRectSize(int height_units = 1);

  void UpdateChildrenPositions();

  int GetLogicalHeightWithChildren() const;

  /**
   * @brief Reference to attached Node
   */
  Node *node_;
  QString input_;
  int element_;

  Node *context_;

  /**
   * @brief Cached list of node inputs
   */
  QVector<NodeViewItem*> children_;

  /// Sizing variables to use when drawing
  int node_border_width_;

  /**
   * @brief Expanded state
   */
  bool expanded_;

  bool highlighted_;

  NodeViewCommon::FlowDirection flow_dir_;

  QVector<NodeViewEdge*> edges_;

  QPointF cached_node_pos_;

  QRect last_arrow_rect_;
  bool arrow_click_;

  NodeViewItemConnector *input_connector_;
  NodeViewItemConnector *output_connector_;

  bool has_connectable_inputs_;

  bool label_as_output_;

private slots:
  void NodeAppearanceChanged();

  void RepopulateInputs();

  void InputArraySizeChanged(const QString &input);

};

}

#endif // NODEVIEWITEM_H
