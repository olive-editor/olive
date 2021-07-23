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
#include "nodeviewconnector.h"

namespace olive {

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
   * @brief Returns GLOBAL point that edges should connect to for any NodeParam member of this object
   */
  QPointF GetInputPoint(const QString& input, int element) const;

  QPointF GetOutputPoint(const QString &output) const;

  /**
   * @brief Sets the direction nodes are flowing
   */
  void SetFlowDirection(NodeViewCommon::FlowDirection dir);

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

  void SetPreventRemoving(bool e)
  {
    prevent_removing_ = e;
  }

  bool GetPreventRemoving() const
  {
    return prevent_removing_;
  }

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
  virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

  virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) override;

private:
  void ReadjustAllEdges();

  void DrawNodeTitle(QPainter *painter, QString text, const QRectF &rect, Qt::Alignment vertical_align, int icon_size, bool draw_arrow);

  /**
   * @brief Internal update function when logical position changes
   */
  void UpdateNodePosition();

  void UpdateInputPositions();

  void UpdateOutputPositions();

  void AddConnector(QVector<NodeViewConnector*> *connectors, const QString &id, int index, bool is_input);
  void RemoveConnector(QVector<NodeViewConnector*> *connectors, const QString &id);
  void UpdateConnectorPositions(QVector<NodeViewConnector*> *connectors, bool top);

  /**
   * @brief Reference to attached Node
   */
  Node* node_;

  /// Sizing variables to use when drawing
  int node_border_width_;

  NodeViewCommon::FlowDirection flow_dir_;

  QVector<NodeViewEdge*> edges_;

  QPointF cached_node_pos_;

  bool prevent_removing_;

  QVector<NodeViewConnector*> input_connectors_;

  QVector<NodeViewConnector*> output_connectors_;

private slots:
  void InputAdded(const QString &id, int index = -1);

  void InputRemoved(const QString &id);

  void OutputAdded(const QString &id, int index = -1);

  void OutputRemoved(const QString &id);

};

}

#endif // NODEVIEWITEM_H
