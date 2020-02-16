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

#ifndef NODEVIEW_H
#define NODEVIEW_H

#include <QGraphicsView>
#include <QTimer>

#include "node/graph.h"
#include "widget/nodeview/nodeviewedge.h"
#include "widget/nodeview/nodeviewitem.h"

/**
 * @brief A widget for viewing and editing node graphs
 *
 * This widget takes a NodeGraph object and constructs a QGraphicsScene representing its data, viewing and allowing
 * the user to make modifications to it.
 */
class NodeView : public QGraphicsView
{
  Q_OBJECT
public:
  NodeView(QWidget* parent);

  virtual ~NodeView() override;

  /**
   * @brief Sets the graph to view
   */
  void SetGraph(NodeGraph* graph);

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
  static NodeViewItem* NodeToUIObject(QGraphicsScene* scene, Node* n);

  /**
   * @brief Retrieve the graphical widget corresponding to a specific NodeEdge
   *
   * Same as NodeToUIObject() but returns a NodeViewEdge corresponding to a NodeEdgePtr instead.
   */
  static NodeViewEdge* EdgeToUIObject(QGraphicsScene* scene, NodeEdgePtr n);

  /**
   * @brief Overloaded NodeToUIObject(QGraphicsScene* scene, Node* n) if you have direct access to a NodeView instance
   */
  NodeViewItem* NodeToUIObject(Node* n);

  /**
   * @brief Overloaded EdgeToUIObject(QGraphicsScene* scene, NodeEdgePtr n) if you have direct access to a NodeView instance
   */
  NodeViewEdge* EdgeToUIObject(NodeEdgePtr n);

  /**
   * @brief Delete selected nodes from graph (user-friendly/undoable)
   */
  void DeleteSelected();

signals:
  /**
   * @brief Signal emitted when the selected nodes have changed
   */
  void SelectionChanged(QList<Node*> selected_nodes);

protected:
  virtual void keyPressEvent(QKeyEvent *event) override;

  virtual void mousePressEvent(QMouseEvent *event) override;

  virtual void mouseMoveEvent(QMouseEvent *event) override;

private:
  QList<Node*> GetNodeDirectDescendants(Node* n, const QList<Node*> connected_nodes, QList<Node*>& processed_nodes);

  void PlaceNode(NodeViewItem* n, const QPointF& pos);

  int FindWeightsInternal(Node* node, QHash<Node*, int>& weights, QList<Node*>& weighted_nodes);

  void ReorganizeInternal(NodeViewItem *src_item, QHash<Node*, int>& weights, QList<Node *> &positioned_nodes);

  void QueueReorganize();

  void AttachItemToCursor(NodeViewItem* item);

  void DetachItemFromCursor();

  NodeGraph* graph_;

  NodeViewItem* attached_item_;

  NodeViewEdge* drop_edge_;
  NodeInput* drop_compatible_input_;

  QGraphicsScene scene_;

  QTimer reorganize_timer_;

private slots:
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

  /**
   * @brief Slot when an edge is added to a graph (SetGraph() connects this)
   *
   * This should NEVER be called directly, only connected to a NodeGraph. To add an edge (i.e. connect two node
   * parameters together), use NodeParam::ConnectEdge().
   */
  void AddEdge(NodeEdgePtr edge);

  /**
   * @brief Slot when an edge is removed from a graph (SetGraph() connects this)
   *
   * This should NEVER be called directly, only connected to a NodeGraph. To remove an edge (i.e. disconnect two node
   * parameters), use NodeParam::DisconnectEdge().
   */
  void RemoveEdge(NodeEdgePtr edge);

  /**
   * @brief Internal function triggered when any change is signalled from the QGraphicsScene
   *
   * Current primary function is to inform all NodeViewEdges to re-adjust in case any Nodes have moved
   */
  void ItemsChanged();

  /**
   * @brief Receiver for when the scene's selected items change
   */
  void SceneSelectionChangedSlot();

  /**
   * @brief Receiver for when the user right clicks (or otherwise requests a context menu)
   */
  void ShowContextMenu(const QPoint &pos);

  /**
   * @brief Receiver for when the user requests a new node from the add menu
   */
  void CreateNodeSlot(QAction* action);

  /**
   * @brief Automatically reposition the nodes based on their connections
   */
  void Reorganize();

};

#endif // NODEVIEW_H
