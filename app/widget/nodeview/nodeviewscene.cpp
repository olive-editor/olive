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

#include "nodeviewscene.h"

#include "common/functiontimer.h"
#include "core.h"
#include "node/project/sequence/sequence.h"
#include "nodeviewedge.h"
#include "nodeviewitem.h"

namespace olive {

NodeViewScene::NodeViewScene(QObject *parent) :
  QGraphicsScene(parent),
  direction_(NodeViewCommon::kLeftToRight),
  curved_edges_(true)
{
}

void NodeViewScene::SetFlowDirection(NodeViewCommon::FlowDirection direction)
{
  direction_ = direction;

  foreach (NodeViewContext *ctx, context_map_) {
    ctx->SetFlowDirection(direction_);
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

  for (auto it=item_map_.cbegin(); it!=item_map_.cend(); it++) {
    delete it.value();
  }
  item_map_.clear();
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

void NodeViewScene::DeleteSelected()
{
  NodeViewDeleteCommand* command = new NodeViewDeleteCommand();

  foreach (NodeViewContext *ctx, context_map_) {
    ctx->DeleteSelected(command);
  }

  Core::instance()->undo_stack()->push(command);
}

NodeViewItem *NodeViewScene::NodeToUIObject(Node *n)
{
  return item_map_.value(n);
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
  QVector<NodeViewItem *> items;

  foreach (NodeViewContext *ctx, context_map_) {
    items.append(ctx->GetSelectedItems());
  }

  return items;
}

NodeViewContext *NodeViewScene::AddContext(Node *node)
{
  NodeViewContext *context_item = context_map_.value(node);

  if (!context_item) {
    context_item = new NodeViewContext(node);

    context_item->SetFlowDirection(GetFlowDirection());
    context_item->SetCurvedEdges(GetEdgesAreCurved());

    QPointF pos(0, 0);
    QRectF item_rect = context_item->rect();
    while (!items(item_rect).isEmpty()) {
      pos.setY(pos.y() + item_rect.height());
      item_rect = context_item->rect().translated(pos);
    }
    context_item->setPos(pos);

    addItem(context_item);

    context_map_.insert(node, context_item);
  }

  return context_item;
}

void NodeViewScene::RemoveContext(Node *node)
{
  delete context_map_.take(node);
}

int NodeViewScene::DetermineWeight(Node *n)
{
  QVector<Node*> inputs = n->GetImmediateDependencies();

  int weight = 0;

  foreach (Node* i, inputs) {
    if (i->GetNumberOfRoutesTo(n) == 1) {
      weight += DetermineWeight(i);
    }
  }

  return qMax(1, weight);
}

Qt::Orientation NodeViewScene::GetFlowOrientation() const
{
  return NodeViewCommon::GetFlowOrientation(direction_);
}

NodeViewCommon::FlowDirection NodeViewScene::GetFlowDirection() const
{
  return direction_;
}

void NodeViewScene::SetEdgesAreCurved(bool curved)
{
  if (curved_edges_ != curved) {
    curved_edges_ = curved;

    foreach (NodeViewContext *ctx, context_map_) {
      ctx->SetCurvedEdges(curved_edges_);
    }
  }
}

}
