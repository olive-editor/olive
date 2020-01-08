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

  Reorganize();
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

  Reorganize();
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

void NodeView::PlaceNode(NodeViewItem *n, const QPointF &pos)
{
  QRectF destination_rect = n->rect();
  destination_rect.translate(n->pos());

  double x_movement = destination_rect.width() * 1.5;
  double y_movement = destination_rect.height() * 1.5;

  QList<QGraphicsItem*> items = scene()->items(destination_rect);

  n->setPos(pos);

  foreach (QGraphicsItem* item, items) {
    if (item == n) {
      continue;
    }

    NodeViewItem* node_item = dynamic_cast<NodeViewItem*>(item);

    if (!node_item) {
      continue;
    }

    qDebug() << "Moving" << node_item->node() << "for" << n->node();

    QPointF new_pos;

    if (item->pos() == pos) {
      qDebug() << "Same pos, need more info";

      // Item positions are exact, we'll need more information to determine where this item should go
      Node* ours = n->node();
      Node* theirs = node_item->node();

      bool moved = false;

      new_pos = item->pos();

      // Heuristic to determine whether to move the other item above or below
      foreach (NodeEdgePtr our_edge, ours->output()->edges()) {
        foreach (NodeEdgePtr their_edge, theirs->output()->edges()) {
          if (our_edge->output()->parentNode() == their_edge->output()->parentNode()) {
            qDebug() << "  They share a node that they output to";
            if (our_edge->input()->index() > their_edge->input()->index()) {
              // Their edge should go above ours
              qDebug() << "    Our edge goes BELOW theirs";
              new_pos.setY(new_pos.y() - y_movement);
            } else {
              // Our edge should go below ours
              qDebug() << "    Our edge goes ABOVE theirs";
              new_pos.setY(new_pos.y() + y_movement);
            }

            moved = true;

            break;
          }
        }
      }

      // If we find anything, just move at random
      if (!moved) {
        new_pos.setY(new_pos.y() - y_movement);
      }

    } else if (item->pos().x() == pos.x()) {
      qDebug() << "Same X, moving vertically";

      // Move strictly up or down
      new_pos = item->pos();

      if (item->pos().y() < pos.y()) {
        // Move further up
        new_pos.setY(pos.y() - y_movement);
      } else {
        // Move further down
        new_pos.setY(pos.y() + y_movement);
      }
    } else if (item->pos().y() == pos.y()) {
      qDebug() << "Same Y, moving horizontally";

      // Move strictly left or right
      new_pos = item->pos();

      if (item->pos().x() < pos.x()) {
        // Move further up
        new_pos.setX(pos.x() - x_movement);
      } else {
        // Move further down
        new_pos.setX(pos.x() + x_movement);
      }
    } else {
      qDebug() << "Diff pos, pushing in angle";

      // The item does not have equal X or Y, attempt to push it away from `pos` in the direction it's in
      double x_diff = item->pos().x() - pos.x();
      double y_diff = item->pos().y() - pos.y();

      double slope = y_diff / x_diff;
      double y_int = item->pos().y() - slope * item->pos().x();

      if (qAbs(slope) > 1.0) {
        // Vertical difference is greater than horizontal difference, prioritize vertical movement
        double desired_y = pos.y();

        if (item->pos().y() > pos.y()) {
          desired_y += y_movement;
        } else {
          desired_y -= y_movement;
        }

        double x = (desired_y - y_int) / slope;

        new_pos = QPointF(x, desired_y);
      } else {
        // Horizontal difference is greater than vertical difference, prioritize horizontal movement
        double desired_x = pos.x();

        if (item->pos().x() > pos.x()) {
          desired_x += x_movement;
        } else {
          desired_x -= x_movement;
        }

        double y = slope * desired_x + y_int;

        new_pos = QPointF(desired_x, y);
      }
    }

    PlaceNode(node_item, new_pos);
  }
}

void NodeView::ReorganizeInternal(NodeViewItem* src_item, QList<Node*>& positioned_nodes)
{
  Node* n = src_item->node();

  QVector<Node*> connected_nodes;

  foreach (NodeParam* param, n->parameters()) {
    if (param->type() == NodeParam::kInput) {
      NodeInput* input = static_cast<NodeInput*>(param);

      if (input->IsConnected() && !connected_nodes.contains(input->get_connected_node())) {
        connected_nodes.append(input->get_connected_node());
      }

      if (input->IsArray()) {
        NodeInputArray* array = static_cast<NodeInputArray*>(input);

        foreach (NodeInput* sub, array->sub_params()) {
          if (sub->IsConnected() && !connected_nodes.contains(sub->get_connected_node())) {
            connected_nodes.append(sub->get_connected_node());
          }
        }
      }
    }
  }

  if (connected_nodes.isEmpty()) {
    return;
  }

  QVector<Node*> direct_descendants = connected_nodes;

  positioned_nodes.append(n);

  // Remove any nodes that aren't necessarily attached directly
  for (int i=0;i<direct_descendants.size();i++) {
    Node* connected = direct_descendants.at(i);

    for (int j=1;j<connected->output()->edges().size();j++) {
      Node* this_output_connection = connected->output()->edges().at(j)->input()->parentNode();
      if (!positioned_nodes.contains(this_output_connection)) {
        direct_descendants.removeAt(i);
        i--;
        break;
      }
    }
  }

  qreal center_y = src_item->y();
  qreal total_height = direct_descendants.size() * src_item->rect().height() + (direct_descendants.size()-1) * src_item->rect().height()/2;
  double item_top = center_y - (total_height/2) + src_item->rect().height()/2;

  // Set each node's position
  for (int i=0;i<direct_descendants.size();i++) {
    Node* connected = direct_descendants.at(i);

    NodeViewItem* item = NodeToUIObject(connected);

    if (!item) {
      continue;
    }

    double item_y = item_top;

    // Multiply the index by the item height (with 1.5 for padding)
    item_y += i * src_item->rect().height() * 1.5;

    QPointF item_pos(src_item->pos().x() - item->rect().width() * 3 / 2,
                     item_y);

//    PlaceNode(item, item_pos);
    item->setPos(item_pos);
  }

  // Recursively work on each node
  foreach (Node* connected, connected_nodes) {
    NodeViewItem* item = NodeToUIObject(connected);

    if (!item) {
      continue;
    }

    ReorganizeInternal(item, positioned_nodes);
  }
}

void NodeView::Reorganize()
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

  QList<Node*> positioned_nodes;
  foreach (Node* end_node, end_nodes) {
    ReorganizeInternal(NodeToUIObject(end_node), positioned_nodes);
  }
}
