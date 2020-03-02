#ifndef NODEVIEWSCENE_H
#define NODEVIEWSCENE_H

#include <QGraphicsScene>
#include <QTimer>

#include "node/graph.h"
#include "widget/nodeview/nodeviewedge.h"
#include "widget/nodeview/nodeviewitem.h"

class NodeViewScene : public QGraphicsScene
{
  Q_OBJECT
public:
  NodeViewScene(QObject *parent = nullptr);

  void clear();

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

  /**
   * @brief Retrieve the graphical widget corresponding to a specific NodeEdge
   *
   * Same as NodeToUIObject() but returns a NodeViewEdge corresponding to a NodeEdgePtr instead.
   */
  NodeViewEdge* EdgeToUIObject(NodeEdgePtr n);

  void SetGraph(NodeGraph* graph);

  const QHash<Node*, NodeViewItem*>& item_map() const;
  const QHash<NodeEdge*, NodeViewEdge*>& edge_map() const;

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

private:
  void QueueReorganize();

  QList<Node*> GetNodeDirectDescendants(Node* n, const QList<Node*> connected_nodes, QList<Node*>& processed_nodes);

  int FindWeightsInternal(Node* node, QHash<Node*, int>& weights, QList<Node*>& weighted_nodes);

  void ReorganizeInternal(NodeViewItem *src_item, QHash<Node*, int>& weights, QList<Node *> &positioned_nodes);

  QHash<Node*, NodeViewItem*> item_map_;

  QHash<NodeEdge*, NodeViewEdge*> edge_map_;

  QTimer reorganize_timer_;

  NodeGraph* graph_;

private slots:
  /**
   * @brief Automatically reposition the nodes based on their connections
   */
  void Reorganize();

};

#endif // NODEVIEWSCENE_H
