#include "nodeviewscene.h"

NodeViewScene::NodeViewScene(QObject *parent) :
  QGraphicsScene(parent),
  graph_(nullptr)
{
  connect(&reorganize_timer_, &QTimer::timeout, &reorganize_timer_, &QTimer::stop);
  connect(&reorganize_timer_, &QTimer::timeout, this, &NodeViewScene::Reorganize);
}

void NodeViewScene::clear()
{
  {
    QHash<Node*, NodeViewItem*>::const_iterator i;
    for (i=item_map_.begin();i!=item_map_.end();i++) {
      delete i.value();
    }
    item_map_.clear();
  }

  {
    QHash<NodeEdge*, NodeViewEdge*>::const_iterator i;
    for (i=edge_map_.begin();i!=edge_map_.end();i++) {
      delete i.value();
    }
    edge_map_.clear();
  }
}

NodeViewItem *NodeViewScene::NodeToUIObject(Node *n)
{
  return item_map_.value(n);
}

NodeViewEdge *NodeViewScene::EdgeToUIObject(NodeEdgePtr n)
{
  return edge_map_.value(n.get());
}

void NodeViewScene::SetGraph(NodeGraph *graph)
{
  graph_ = graph;
}

const QHash<Node *, NodeViewItem *> &NodeViewScene::item_map() const
{
  return item_map_;
}

void NodeViewScene::AddNode(Node* node)
{
  NodeViewItem* item = new NodeViewItem();

  item->SetNode(node);

  addItem(item);
  item_map_.insert(node, item);

  // Add a NodeViewEdge for each connection
  foreach (NodeParam* param, node->parameters()) {

    // We only bother working with outputs since eventually this will cover all inputs too
    // (covering both would lead to duplicates since every edge connects to one input and one output)
    if (param->type() == NodeParam::kOutput) {
      const QVector<NodeEdgePtr>& edges = param->edges();

      foreach(NodeEdgePtr edge, edges) {
        AddEdge(edge);
      }
    }
  }

  QueueReorganize();
}

void NodeViewScene::RemoveNode(Node *node)
{
  delete item_map_.take(node);
}

void NodeViewScene::AddEdge(NodeEdgePtr edge)
{
  NodeViewEdge* edge_ui = new NodeViewEdge();

  edge_ui->SetEdge(edge);

  addItem(edge_ui);
  edge_map_.insert(edge.get(), edge_ui);

  QueueReorganize();
}

void NodeViewScene::RemoveEdge(NodeEdgePtr edge)
{
  delete edge_map_.take(edge.get());
}

void NodeViewScene::QueueReorganize()
{
  // Avoids the fairly complex Reorganize() function every single time a connection or node is added

  reorganize_timer_.stop();
  reorganize_timer_.start(20);
}

QList<Node *> NodeViewScene::GetNodeDirectDescendants(Node* n, const QList<Node*> connected_nodes, QList<Node*>& processed_nodes)
{
  QList<Node*> direct_descendants = connected_nodes;

  processed_nodes.append(n);

  // Remove any nodes that aren't necessarily attached directly
  for (int i=0;i<direct_descendants.size();i++) {
    Node* connected = direct_descendants.at(i);

    for (int j=1;j<connected->output()->edges().size();j++) {
      Node* this_output_connection = connected->output()->edges().at(j)->input()->parentNode();
      if (!processed_nodes.contains(this_output_connection)) {
        direct_descendants.removeAt(i);
        i--;
        break;
      }
    }
  }

  return direct_descendants;
}

int NodeViewScene::FindWeightsInternal(Node *node, QHash<Node *, int> &weights, QList<Node*>& weighted_nodes)
{
  QList<Node*> connected_nodes = node->GetImmediateDependencies();

  int weight = 0;

  if (!connected_nodes.isEmpty()) {
    QList<Node*> direct_descendants = GetNodeDirectDescendants(node, connected_nodes, weighted_nodes);

    foreach (Node* dep, direct_descendants) {
      weight += FindWeightsInternal(dep, weights, weighted_nodes);
    }
  }

  weight = qMax(weight, 1);

  weights.insert(node, weight);

  return weight;
}

void NodeViewScene::ReorganizeInternal(NodeViewItem* src_item, QHash<Node*, int>& weights, QList<Node*>& positioned_nodes)
{
  if (!src_item) {
    return;
  }

  Node* n = src_item->node();

  QList<Node*> connected_nodes = n->GetImmediateDependencies();

  if (connected_nodes.isEmpty()) {
    return;
  }

  QList<Node*> direct_descendants = GetNodeDirectDescendants(n, connected_nodes, positioned_nodes);

  int descendant_weight = 0;
  foreach (Node* dep, direct_descendants) {
    descendant_weight += weights.value(dep);
  }

  qreal center_y = src_item->y();
  qreal total_height = descendant_weight * src_item->rect().height() + (direct_descendants.size()-1) * src_item->rect().height()/2;
  double item_top = center_y - (total_height/2) + src_item->rect().height()/2;

  // Set each node's position
  int weight_index = 0;
  for (int i=0;i<direct_descendants.size();i++) {
    Node* connected = direct_descendants.at(i);

    NodeViewItem* item = NodeToUIObject(connected);

    if (!item) {
      continue;
    }

    double item_y = item_top;

    // Multiply the index by the item height (with 1.5 for padding)
    item_y += weight_index * src_item->rect().height() * 1.5;

    QPointF item_pos(src_item->pos().x() - item->rect().width() * 3 / 2,
                     item_y);

    item->setPos(item_pos);

    weight_index += weights.value(connected);
  }

  // Recursively work on each node
  foreach (Node* connected, connected_nodes) {
    NodeViewItem* item = NodeToUIObject(connected);

    if (!item) {
      continue;
    }

    ReorganizeInternal(item, weights, positioned_nodes);
  }
}

void NodeViewScene::Reorganize()
{
  if (!graph_) {
    return;
  }

  QList<Node*> end_nodes;

  // Calculate the nodes that don't output to anything, they'll be our anchors
  foreach (Node* node, graph_->nodes()) {
    if (!node->HasConnectedOutputs()) {
      end_nodes.append(node);
    }
  }

  QList<Node*> processed_nodes;

  QHash<Node*, int> node_weights;
  foreach (Node* end_node, end_nodes) {
    FindWeightsInternal(end_node, node_weights, processed_nodes);
  }

  processed_nodes.clear();

  foreach (Node* end_node, end_nodes) {
    ReorganizeInternal(NodeToUIObject(end_node), node_weights, processed_nodes);
  }
}
