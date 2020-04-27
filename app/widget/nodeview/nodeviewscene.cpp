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

#include "nodeviewscene.h"

#include "nodeviewedge.h"
#include "nodeviewitem.h"

OLIVE_NAMESPACE_ENTER

NodeViewScene::NodeViewScene(QObject *parent) :
  QGraphicsScene(parent),
  graph_(nullptr),
  direction_(NodeViewCommon::kLeftToRight)
{
}

void NodeViewScene::SetFlowDirection(NodeViewCommon::FlowDirection direction)
{
  direction_ = direction;

  {
    QHash<Node*, NodeViewItem*>::const_iterator i;
    for (i=item_map_.constBegin(); i!=item_map_.constEnd(); i++) {
      i.value()->SetFlowDirection(direction_);
    }
  }

  {
    QHash<NodeEdge*, NodeViewEdge*>::const_iterator i;
    for (i=edge_map_.constBegin(); i!=edge_map_.constEnd(); i++) {
      i.value()->SetFlowDirection(direction_);
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

NodeViewEdge *NodeViewScene::EdgeToUIObject(NodeEdgePtr n)
{
  return edge_map_.value(n.get());
}

void NodeViewScene::SetGraph(NodeGraph *graph)
{
  graph_ = graph;
}

QList<Node *> NodeViewScene::GetSelectedNodes() const
{
  QHash<Node*, NodeViewItem*>::const_iterator iterator;
  QList<Node *> selected;

  for (iterator=item_map_.begin();iterator!=item_map_.end();iterator++) {
    if (iterator.value()->isSelected()) {
      selected.append(iterator.key());
    }
  }

  return selected;
}

QList<NodeViewItem *> NodeViewScene::GetSelectedItems() const
{
  QHash<Node*, NodeViewItem*>::const_iterator iterator;
  QList<NodeViewItem *> selected;

  for (iterator=item_map_.begin();iterator!=item_map_.end();iterator++) {
    if (iterator.value()->isSelected()) {
      selected.append(iterator.value());
    }
  }

  return selected;
}

const QHash<Node *, NodeViewItem *> &NodeViewScene::item_map() const
{
  return item_map_;
}

const QHash<NodeEdge *, NodeViewEdge *> &NodeViewScene::edge_map() const
{
  return edge_map_;
}

void NodeViewScene::AddNode(Node* node)
{
  NodeViewItem* item = new NodeViewItem();

  item->SetNode(node);
  item->SetFlowDirection(direction_);

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
}

void NodeViewScene::RemoveNode(Node *node)
{
  delete item_map_.take(node);
}

void NodeViewScene::AddEdge(NodeEdgePtr edge)
{
  NodeViewEdge* edge_ui = new NodeViewEdge();

  edge_ui->SetEdge(edge);
  edge_ui->SetFlowDirection(direction_);

  addItem(edge_ui);
  edge_map_.insert(edge.get(), edge_ui);
}

void NodeViewScene::RemoveEdge(NodeEdgePtr edge)
{
  delete edge_map_.take(edge.get());
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
  QList<Node*> immediates = n->GetImmediateDependencies();

  if (immediates.isEmpty()) {
    // Nothing to do
    return;
  }

  NodeViewItem* parent_item = NodeToUIObject(n);

  int item_sz, item_padding, layer_diff, total_top;

  if (GetFlowOrientation() == Qt::Vertical) {
    item_sz = NodeViewItem::DefaultItemWidth();
    item_padding = item_sz / 2;
    layer_diff = NodeViewItem::DefaultItemHeight() * 2;
    total_top = parent_item->x();
  } else {
    item_sz = NodeViewItem::DefaultItemHeight();
    item_padding = item_sz;
    layer_diff = NodeViewItem::DefaultItemWidth() * 3 / 2;
    total_top = parent_item->y();
  }

  int item_sz_with_padding = item_sz + item_padding;

  int total_sz = item_sz_with_padding * immediates.size() - item_padding;

  total_top -= total_sz / 2;
  total_top += item_sz / 2;

  int item_layer_pos;

  switch (direction_) {
  case NodeViewCommon::kTopToBottom:
    item_layer_pos = parent_item->pos().y() - layer_diff;
    break;
  case NodeViewCommon::kLeftToRight:
    item_layer_pos = parent_item->pos().x() - layer_diff;
    break;
  case NodeViewCommon::kBottomToTop:
    item_layer_pos = parent_item->pos().y() + layer_diff;
    break;
  case NodeViewCommon::kRightToLeft:
    item_layer_pos = parent_item->pos().x() + layer_diff;
    break;
  }

  for (int i=0;i<immediates.size();i++) {
    NodeViewItem* item = NodeToUIObject(immediates.at(i));

    if (GetFlowOrientation() == Qt::Vertical) {
      item->setPos(total_top + item_sz_with_padding * i,
                   item_layer_pos);
    } else {
      item->setPos(item_layer_pos,
                   total_top + item_sz_with_padding * i);
    }

    ReorganizeFrom(immediates.at(i));
  }
}

OLIVE_NAMESPACE_EXIT
