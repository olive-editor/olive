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

#include "nodeviewscene.h"

#include "common/functiontimer.h"
#include "nodeviewedge.h"
#include "nodeviewitem.h"
#include "project/item/sequence/sequence.h"

namespace olive {

NodeViewScene::NodeViewScene(QObject *parent) :
  QGraphicsScene(parent),
  graph_(nullptr),
  direction_(NodeViewCommon::kLeftToRight),
  curved_edges_(true)
{
}

void NodeViewScene::SetFlowDirection(NodeViewCommon::FlowDirection direction)
{
  direction_ = direction;

  {
    // Iterate over node items setting direction
    QHash<Node*, NodeViewItem*>::const_iterator i;
    for (i=item_map_.constBegin(); i!=item_map_.constEnd(); i++) {
      i.value()->SetFlowDirection(direction_);

      // Update position too
      i.value()->SetNodePosition(i.key()->GetPosition());
    }
  }

  {
    // Iterate over edge items setting direction
    foreach (NodeViewEdge* edge, edges_) {
      edge->SetFlowDirection(direction_);
    }
  }
}

void NodeViewScene::clear()
{
  // Deselect everything (prevents signals that a selection has changed after deleting an object)
  DeselectAll();

  // HACK: QGraphicsScene contains some sort of internal caching of the selected items which doesn't update unless
  //       we call a function like this. That means even though we deselect all items above, QGraphicsScene will
  //       continue to incorrectly signal selectionChanged() when items that were selected (but are now not) get
  //       deleted. Calling this function appears to update the internal cache and prevent this.
  selectedItems();

  qDeleteAll(item_map_);
  item_map_.clear();

  qDeleteAll(edges_);
  edges_.clear();
}

void NodeViewScene::SelectAll()
{
  QList<QGraphicsItem *> all_items = this->items();

  foreach (QGraphicsItem* i, all_items) {
    i->setSelected(true);
  }
}

void NodeViewScene::DeselectAll()
{
  QList<QGraphicsItem *> selected_items = this->selectedItems();

  foreach (QGraphicsItem* i, selected_items) {
    i->setSelected(false);
  }
}

NodeViewItem *NodeViewScene::NodeToUIObject(Node *n)
{
  return item_map_.value(n);
}

NodeViewEdge *NodeViewScene::EdgeToUIObject(Node* output, NodeInput* input, int element)
{
  foreach (NodeViewEdge* edge, edges_) {
    if (edge->output() == output && edge->input() == input && edge->element() == element) {
      return edge;
    }
  }

  return nullptr;
}

void NodeViewScene::SetGraph(NodeGraph *graph)
{
  graph_ = graph;
}

QVector<Node *> NodeViewScene::GetSelectedNodes() const
{
  QHash<Node*, NodeViewItem*>::const_iterator iterator;
  QVector<Node *> selected;

  for (iterator=item_map_.begin();iterator!=item_map_.end();iterator++) {
    if (iterator.value()->isSelected()) {
      selected.append(iterator.key());
    }
  }

  return selected;
}

QVector<NodeViewItem *> NodeViewScene::GetSelectedItems() const
{
  QHash<Node*, NodeViewItem*>::const_iterator iterator;
  QVector<NodeViewItem *> selected;

  for (iterator=item_map_.begin();iterator!=item_map_.end();iterator++) {
    if (iterator.value()->isSelected()) {
      selected.append(iterator.value());
    }
  }

  return selected;
}

QVector<NodeViewEdge *> NodeViewScene::GetSelectedEdges() const
{
  QVector<NodeViewEdge*> edges;

  foreach (NodeViewEdge* e, edges_) {
    if (e->isSelected()) {
      edges.append(e);
    }
  }

  return edges;
}

void NodeViewScene::AddNode(Node* node)
{
  NodeViewItem* item = new NodeViewItem();

  item->SetFlowDirection(direction_);
  item->SetNode(node);

  addItem(item);
  item_map_.insert(node, item);

  connect(node, &Node::PositionChanged, this, &NodeViewScene::NodePositionChanged);
  connect(node, &Node::LabelChanged, this, &NodeViewScene::NodeAppearanceChanged);
  connect(node, &Node::ColorChanged, this, &NodeViewScene::NodeAppearanceChanged);
}

void NodeViewScene::RemoveNode(Node *node)
{
  disconnect(node, &Node::ColorChanged, this, &NodeViewScene::NodeAppearanceChanged);
  disconnect(node, &Node::LabelChanged, this, &NodeViewScene::NodeAppearanceChanged);
  disconnect(node, &Node::PositionChanged, this, &NodeViewScene::NodePositionChanged);

  delete item_map_.take(node);
}

void NodeViewScene::AddEdge(Node* output, NodeInput* input, int element)
{
  AddEdgeInternal(output, input, element, NodeToUIObject(output), NodeToUIObject(input->parent()));
}

void NodeViewScene::RemoveEdge(Node* output, NodeInput* input, int element)
{
  NodeViewEdge* edge = EdgeToUIObject(output, input, element);
  edge->from_item()->RemoveEdge(edge);
  edge->to_item()->RemoveEdge(edge);
  edges_.removeOne(edge);
  delete edge;
}

int NodeViewScene::DetermineWeight(Node *n)
{
  QVector<Node*> inputs = n->GetImmediateDependencies();

  int weight = 0;

  foreach (Node* i, inputs) {
    if (i->GetRoutesTo(n) == 1) {
      weight += DetermineWeight(i);
    }
  }

  return qMax(1, weight);
}

void NodeViewScene::AddEdgeInternal(Node *output, NodeInput *input, int element, NodeViewItem *from, NodeViewItem *to)
{
  NodeViewEdge* edge_ui = new NodeViewEdge(output, input, element, from, to);

  edge_ui->SetFlowDirection(direction_);
  edge_ui->SetCurved(curved_edges_);

  from->AddEdge(edge_ui);
  to->AddEdge(edge_ui);

  addItem(edge_ui);
  edges_.append(edge_ui);
}

Qt::Orientation NodeViewScene::GetFlowOrientation() const
{
  return NodeViewCommon::GetFlowOrientation(direction_);
}

NodeViewCommon::FlowDirection NodeViewScene::GetFlowDirection() const
{
  return direction_;
}

void NodeViewScene::ReorganizeFrom(Node* n)
{
  QVector<Node*> immediates = n->GetImmediateDependencies();

  if (immediates.isEmpty()) {
    // Nothing to do
    return;
  }

  QPointF parent_pos = n->GetPosition();

  int weight_count = DetermineWeight(n);

  qreal child_x = parent_pos.x() - 1.0;
  qreal children_height = weight_count-1;
  qreal children_y = parent_pos.y() - children_height * 0.5;

  int weight_counter = 0;

  foreach (Node* i, immediates) {
    if (i->GetRoutesTo(n) == 1) {
      int weight = DetermineWeight(i);

      i->SetPosition(QPointF(child_x,
                             children_y + weight_counter + (weight - 1) * 0.5));

      weight_counter += weight;

      ReorganizeFrom(i);
    }
  }
}

bool NodeViewScene::GetEdgesAreCurved() const
{
  return curved_edges_;
}

void NodeViewScene::SetEdgesAreCurved(bool curved)
{
  if (curved_edges_ != curved) {
    curved_edges_ = curved;

    foreach (NodeViewEdge* e, edges_) {
      e->SetCurved(curved_edges_);
    }
  }
}

void NodeViewScene::NodePositionChanged(const QPointF &pos)
{
  // Update node's internal position
  item_map_.value(static_cast<Node*>(sender()))->SetNodePosition(pos);
}

void NodeViewScene::NodeAppearanceChanged()
{
  // Force item to update
  item_map_.value(static_cast<Node*>(sender()))->update();
}

}
