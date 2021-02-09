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

#ifndef NODEVIEWSCENE_H
#define NODEVIEWSCENE_H

#include <QGraphicsScene>
#include <QTimer>

#include "node/graph.h"
#include "nodeviewedge.h"
#include "nodeviewitem.h"

namespace olive {

class NodeViewScene : public QGraphicsScene
{
  Q_OBJECT
public:
  NodeViewScene(QObject *parent = nullptr);

  void clear();

  void SelectAll();
  void DeselectAll();

  /**
   * @brief Retrieve the graphical widget corresponding to a specific Node
   *
   * In situations where you know what Node you're working with but need the UI object (e.g. for positioning), this
   * static function will retrieve the NodeViewItem (Node UI representation) connected to this Node in a certain
   * QGraphicsScene. This can be called from any other UI object, since it'll have a reference to the QGraphicsScene
   * through QGraphicsItem::scene().
   *
   * If the scene does not contain a widget for this node (usually meaning the node's graph is not the active graph
   * in this view/scene), this function returns nullptr.
   */
  NodeViewItem* NodeToUIObject(Node* n);
  NodeViewEdge *EdgeToUIObject(const NodeOutput &output, const NodeInput &input);

  QVector<Node *> GetSelectedNodes() const;
  QVector<NodeViewItem*> GetSelectedItems() const;
  QVector<NodeViewEdge*> GetSelectedEdges() const;

  const QHash<Node*, NodeViewItem*>& item_map() const
  {
    return item_map_;
  }

  const QVector<NodeViewEdge*>& edges() const
  {
    return edges_;
  }

  Qt::Orientation GetFlowOrientation() const;

  NodeViewCommon::FlowDirection GetFlowDirection() const;
  void SetFlowDirection(NodeViewCommon::FlowDirection direction);

  bool GetEdgesAreCurved() const
  {
    return curved_edges_;
  }

  void ReorganizeFrom(Node* n);

public slots:
  /**
   * @brief Slot when a Node is added to a graph (SetGraph() connects this)
   *
   * This should NEVER be called directly, only connected to a NodeGraph. To add a Node to the NodeGraph
   * use NodeGraph::AddNode().
   */
  void AddNode(Node* node);

  /**
   * @brief Slot when a Node is removed from a graph (SetGraph() connects this)
   *
   * This should NEVER be called directly, only connected to a NodeGraph. To remove a Node from the NodeGraph
   * use NodeGraph::RemoveNode().
   */
  void RemoveNode(Node* node);

  void AddEdge(const NodeOutput& output, const NodeInput& input);
  void RemoveEdge(const NodeOutput& output, const NodeInput& input);

  /**
   * @brief Set whether edges in this scene should be curved or not
   */
  void SetEdgesAreCurved(bool curved);

private:
  static int DetermineWeight(Node* n);

  void AddEdgeInternal(const NodeOutput &output, const NodeInput &input, NodeViewItem* from, NodeViewItem* to);

  QHash<Node*, NodeViewItem*> item_map_;

  QVector<NodeViewEdge*> edges_;

  NodeGraph* graph_;

  NodeViewCommon::FlowDirection direction_;

  bool curved_edges_;

private slots:
  /**
   * @brief Receiver for whenever a node position changes
   */
  void NodePositionChanged(const QPointF& pos);

  /**
   * @brief Receiver for when a node's label has changed
   */
  void NodeAppearanceChanged();

};

}

#endif // NODEVIEWSCENE_H
