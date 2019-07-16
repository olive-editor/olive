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

NodeView::NodeView(QWidget *parent) :
  QGraphicsView(parent),
  graph_(nullptr)
{
  setScene(&scene_);

  setDragMode(RubberBandDrag);

  connect(&scene_, SIGNAL(changed(const QList<QRectF>&)), this, SLOT(ItemsChanged()));
}

void NodeView::SetGraph(NodeGraph *graph)
{
  // Clear the scene of all UI objects
  scene_.clear();

  // Set reference to the graph
  graph_ = graph;

  // If the graph is valid, add UI objects for each of its Nodes
  if (graph_ != nullptr) {
    QList<Node*> graph_nodes = graph_->nodes();

    foreach (Node* node, graph_nodes) {
      NodeViewItem* item = new NodeViewItem();

      item->SetNode(node);

      scene_.addItem(item);

      // Add a NodeViewEdge for each connection
      QList<NodeParam*> node_params = node->parameters();

      foreach (NodeParam* param, node_params) {

        // We only bother working with outputs since eventually this will cover all inputs too
        // (covering both would lead to duplicates since every edge connects to one input and one output)
        if (param->type() == NodeParam::kOutput) {
          const QVector<NodeEdgePtr>& edges = param->edges();

          foreach(NodeEdgePtr edge, edges) {
            NodeViewEdge* edge_ui = new NodeViewEdge();

            edge_ui->SetEdge(edge);

            scene_.addItem(edge_ui);
          }
        }
      }
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
