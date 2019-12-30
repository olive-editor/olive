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

#include "nodeview.h"

#include "core.h"
#include "nodeviewundo.h"
#include "node/factory.h"

NodeView::NodeView(QWidget *parent) :
  QGraphicsView(parent),
  graph_(nullptr)
{
  setScene(&scene_);
  setDragMode(RubberBandDrag);
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(&scene_, &QGraphicsScene::changed, this, &NodeView::ItemsChanged);
  connect(&scene_, &QGraphicsScene::selectionChanged, this, &NodeView::SceneSelectionChangedSlot);
  connect(this, &NodeView::customContextMenuRequested, this, &NodeView::ShowContextMenu);
}

NodeView::~NodeView()
{
  // Unset the current graph
  SetGraph(nullptr);
}

void NodeView::SetGraph(NodeGraph *graph)
{
  if (graph_ == graph) {
    return;
  }

  if (graph_ != nullptr) {
    disconnect(graph_, &NodeGraph::NodeAdded, this, &NodeView::AddNode);
    disconnect(graph_, &NodeGraph::NodeRemoved, this, &NodeView::RemoveNode);
    disconnect(graph_, &NodeGraph::EdgeAdded, this, &NodeView::AddEdge);
    disconnect(graph_, &NodeGraph::EdgeRemoved, this, &NodeView::RemoveEdge);
  }

  // Clear the scene of all UI objects
  scene_.clear();

  // Set reference to the graph
  graph_ = graph;

  // If the graph is valid, add UI objects for each of its Nodes
  if (graph_ != nullptr) {
    connect(graph_, &NodeGraph::NodeAdded, this, &NodeView::AddNode);
    connect(graph_, &NodeGraph::NodeRemoved, this, &NodeView::RemoveNode);
    connect(graph_, &NodeGraph::EdgeAdded, this, &NodeView::AddEdge);
    connect(graph_, &NodeGraph::EdgeRemoved, this, &NodeView::RemoveEdge);

    QList<Node*> graph_nodes = graph_->nodes();

    foreach (Node* node, graph_nodes) {
      AddNode(node);
    }
  }
}

NodeViewItem *NodeView::NodeToUIObject(QGraphicsScene *scene, Node *n)
{
  QList<QGraphicsItem*> graphics_items = scene->items();

  for (int i=0;i<graphics_items.size();i++) {
    NodeViewItem* item = dynamic_cast<NodeViewItem*>(graphics_items.at(i));

    if (item != nullptr) {
      if (item->node() == n) {
        return item;
      }
    }
  }

  return nullptr;
}

NodeViewEdge *NodeView::EdgeToUIObject(QGraphicsScene *scene, NodeEdgePtr n)
{
  QList<QGraphicsItem*> graphics_items = scene->items();

  for (int i=0;i<graphics_items.size();i++) {
    NodeViewEdge* edge = dynamic_cast<NodeViewEdge*>(graphics_items.at(i));

    if (edge != nullptr) {
      if (edge->edge() == n) {
        return edge;
      }
    }
  }

  return nullptr;
}

NodeViewItem *NodeView::NodeToUIObject(Node *n)
{
  return NodeToUIObject(&scene_, n);
}

NodeViewEdge *NodeView::EdgeToUIObject(NodeEdgePtr n)
{
  return EdgeToUIObject(&scene_, n);
}

void NodeView::DeleteSelected()
{
  if (!graph_) {
    return;
  }

  QList<QGraphicsItem*> selected = scene_.selectedItems();
  QList<Node*> selected_nodes;

  foreach (QGraphicsItem* item, selected) {
    NodeViewItem* node_item = dynamic_cast<NodeViewItem*>(item);

    if (node_item) {
      selected_nodes.append(node_item->node());
    }
  }

  if (selected_nodes.isEmpty()) {
    return;
  }

  Core::instance()->undo_stack()->push(new NodeRemoveCommand(graph_, selected_nodes));
}

void NodeView::AddNode(Node* node)
{
  NodeViewItem* item = new NodeViewItem();

  item->SetNode(node);

  scene_.addItem(item);

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

void NodeView::RemoveNode(Node *node)
{
  NodeViewItem* item = NodeToUIObject(scene(), node);

  delete item;
}

void NodeView::AddEdge(NodeEdgePtr edge)
{
  NodeViewEdge* edge_ui = new NodeViewEdge();

  edge_ui->SetEdge(edge);

  scene_.addItem(edge_ui);
}

void NodeView::RemoveEdge(NodeEdgePtr edge)
{
  NodeViewEdge* edge_ui = EdgeToUIObject(scene(), edge);

  delete edge_ui;
}

void NodeView::ItemsChanged()
{
  QList<QGraphicsItem*> items = scene_.items();

  foreach (QGraphicsItem* item, items) {
    NodeViewEdge* edge = dynamic_cast<NodeViewEdge*>(item);

    if (edge != nullptr) {
      edge->Adjust();
    }
  }
}

void NodeView::SceneSelectionChangedSlot()
{
  // Get the scene's selected items and convert it into a list of selected nodes
  QList<QGraphicsItem*> selected_items = scene_.selectedItems();

  QList<Node*> selected_nodes;

  for (int i=0;i<selected_items.size();i++) {
    // Try to dynamically cast to a widget holding a Node
    NodeViewItem* item = dynamic_cast<NodeViewItem*>(selected_items.at(i));

    if (item != nullptr) {
      selected_nodes.append(item->node());
    }
  }

  emit SelectionChanged(selected_nodes);
}

void NodeView::ShowContextMenu(const QPoint &pos)
{
  if (!graph_) {
    return;
  }

  Menu* m = new Menu();

  Menu* add_menu = NodeFactory::CreateMenu();
  add_menu->setTitle(tr("Add"));
  connect(add_menu, &Menu::triggered, this, &NodeView::CreateNodeSlot);
  m->addMenu(add_menu);

  m->exec(mapToGlobal(pos));
}

void NodeView::CreateNodeSlot(QAction *action)
{
  Node* new_node = NodeFactory::CreateFromMenuAction(action);

  if (new_node) {
    Core::instance()->undo_stack()->push(new NodeAddCommand(graph_, new_node));
  }
}
