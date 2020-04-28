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

#include <QMouseEvent>

#include "core.h"
#include "nodeviewundo.h"
#include "node/factory.h"

#define super HandMovableView

OLIVE_NAMESPACE_ENTER

NodeView::NodeView(QWidget *parent) :
  HandMovableView(parent),
  graph_(nullptr),
  attached_item_(nullptr),
  drop_edge_(nullptr)
{
  setScene(&scene_);
  setDragMode(RubberBandDrag);
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(&scene_, &QGraphicsScene::changed, this, &NodeView::ItemsChanged);
  connect(&scene_, &QGraphicsScene::selectionChanged, this, &NodeView::SceneSelectionChangedSlot);
  connect(this, &NodeView::customContextMenuRequested, this, &NodeView::ShowContextMenu);

  setMouseTracking(true);
  setRenderHint(QPainter::Antialiasing);
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
    disconnect(graph_, &NodeGraph::NodeAdded, &scene_, &NodeViewScene::AddNode);
    disconnect(graph_, &NodeGraph::NodeRemoved, &scene_, &NodeViewScene::RemoveNode);
    disconnect(graph_, &NodeGraph::EdgeAdded, &scene_, &NodeViewScene::AddEdge);
    disconnect(graph_, &NodeGraph::EdgeRemoved, &scene_, &NodeViewScene::RemoveEdge);
  }

  // Clear the scene of all UI objects
  scene_.clear();

  // Set reference to the graph
  graph_ = graph;
  scene_.SetGraph(graph_);

  // If the graph is valid, add UI objects for each of its Nodes
  if (graph_ != nullptr) {
    connect(graph_, &NodeGraph::NodeAdded, &scene_, &NodeViewScene::AddNode);
    connect(graph_, &NodeGraph::NodeRemoved, &scene_, &NodeViewScene::RemoveNode);
    connect(graph_, &NodeGraph::EdgeAdded, &scene_, &NodeViewScene::AddEdge);
    connect(graph_, &NodeGraph::EdgeRemoved, &scene_, &NodeViewScene::RemoveEdge);

    QList<Node*> graph_nodes = graph_->nodes();

    foreach (Node* node, graph_nodes) {
      scene_.AddNode(node);
    }
  }
}

void NodeView::DeleteSelected()
{
  if (!graph_) {
    return;
  }

  QList<Node*> selected_nodes = scene_.GetSelectedNodes();

  // Ensure no nodes are "undeletable"
  for (int i=0;i<selected_nodes.size();i++) {
    if (!selected_nodes.at(i)->CanBeDeleted()) {
      selected_nodes.removeAt(i);
      i--;
    }
  }

  if (selected_nodes.isEmpty()) {
    return;
  }

  Core::instance()->undo_stack()->push(new NodeRemoveCommand(graph_, selected_nodes));
}

void NodeView::SelectAll()
{
  scene_.SelectAll();
}

void NodeView::DeselectAll()
{
  scene_.DeselectAll();
}

void NodeView::Select(const QList<Node *> &nodes)
{
  if (!graph_) {
    return;
  }

  DeselectAll();

  foreach (Node* n, nodes) {
    NodeViewItem* item = scene_.NodeToUIObject(n);

    item->setSelected(true);
  }
}

void NodeView::SelectWithDependencies(QList<Node *> nodes)
{
  if (!graph_) {
    return;
  }

  int original_length = nodes.size();
  for (int i=0;i<original_length;i++) {
    nodes.append(nodes.at(i)->GetDependencies());
  }

  Select(nodes);
}

void NodeView::CopySelected(bool cut)
{
  if (!graph_) {
    return;
  }

  QList<Node*> selected = scene_.GetSelectedNodes();

  if (selected.isEmpty()) {
    return;
  }

  CopyNodesToClipboard(selected);

  if (cut) {
    DeleteSelected();
  }
}

void NodeView::Paste()
{
  if (!graph_) {
    return;
  }

  QUndoCommand* command = new QUndoCommand();

  QList<Node*> pasted_nodes = PasteNodesFromClipboard(static_cast<Sequence*>(graph_), command);

  Core::instance()->undo_stack()->pushIfHasChildren(command);

  if (!pasted_nodes.isEmpty()) {
    // FIXME: Attach to cursor so user can drop in place
  }
}

void NodeView::ItemsChanged()
{
  QHash<NodeEdge *, NodeViewEdge *>::const_iterator i;

  for (i=scene_.edge_map().begin();i!=scene_.edge_map().end();i++) {
    i.value()->Adjust();
  }
}

void NodeView::keyPressEvent(QKeyEvent *event)
{
  super::keyPressEvent(event);

  if (event->key() == Qt::Key_Escape && attached_item_) {
    DetachItemFromCursor();

    // We undo the last action which SHOULD be adding the node
    // FIXME: Possible danger of this not being the case?
    Core::instance()->undo_stack()->undo();
  }
}

void NodeView::mousePressEvent(QMouseEvent *event)
{
  if (HandPress(event)) return;

  if (attached_item_) {
    Node* dropping_node = attached_item_->GetNode();

    DetachItemFromCursor();

    if (drop_edge_) {
      NodeEdgePtr old_edge = drop_edge_->edge();

      // We have everything we need to place the node in between
      QUndoCommand* command = new QUndoCommand();

      // Remove old edge
      new NodeEdgeRemoveCommand(old_edge, command);

      // Place new edges
      new NodeEdgeAddCommand(old_edge->output(), drop_compatible_input_, command);
      new NodeEdgeAddCommand(dropping_node->output(), old_edge->input(), command);

      Core::instance()->undo_stack()->push(command);
    }

    drop_edge_ = nullptr;
  }

  super::mousePressEvent(event);
}

void NodeView::mouseMoveEvent(QMouseEvent *event)
{
  if (HandMove(event)) return;

  super::mouseMoveEvent(event);

  if (attached_item_) {
    attached_item_->setPos(mapToScene(event->pos()));

    // See if the user clicked on an edge
    QRect edge_detect_rect(event->pos(), event->pos());

    // FIXME: Hardcoded numbers
    edge_detect_rect.adjust(-20, -20, 20, 20);

    QList<QGraphicsItem*> items = this->items(edge_detect_rect);

    NodeViewEdge* new_drop_edge = nullptr;

    foreach (QGraphicsItem* item, items) {
      NodeViewEdge* edge = dynamic_cast<NodeViewEdge*>(item);

      if (edge) {
        // Try to place this node inside this edge

        // See if the node we're dropping has an input of a compatible data type
        NodeInput* edges_input = edge->edge()->input();
        NodeParam::DataType input_type = edges_input->data_type();

        NodeInput* compatible_input = nullptr;

        foreach (NodeParam* drop_node_param, attached_item_->GetNode()->parameters()) {
          if (drop_node_param->type() == NodeParam::kInput
              && static_cast<NodeInput*>(drop_node_param)->data_type() & input_type) {
            compatible_input = static_cast<NodeInput*>(drop_node_param);
            break;
          }
        }

        if (compatible_input) {
          new_drop_edge = edge;
          drop_compatible_input_ = compatible_input;

          break;
        }
      }
    }

    if (drop_edge_ != new_drop_edge) {
      if (drop_edge_) {
        drop_edge_->SetHighlighted(false);
      }

      drop_edge_ = new_drop_edge;

      if (drop_edge_) {
        drop_edge_->SetHighlighted(true);
      }
    }
  }
}

void NodeView::mouseReleaseEvent(QMouseEvent *event)
{
  if (HandRelease(event)) return;

  super::mouseReleaseEvent(event);
}

void NodeView::wheelEvent(QWheelEvent *event)
{
  if (event->modifiers() & Qt::ControlModifier) {
    // FIXME: Hardcoded divider (0.001)
    qreal multiplier = 1.0 + (static_cast<qreal>(event->delta()) * 0.001);

    scale(multiplier, multiplier);
  } else {
    QWidget::wheelEvent(event);
  }
}

void NodeView::SceneSelectionChangedSlot()
{
  emit SelectionChanged(scene_.GetSelectedNodes());
}

void NodeView::ShowContextMenu(const QPoint &pos)
{
  if (!graph_) {
    return;
  }

  Menu m;

  Menu* add_menu = NodeFactory::CreateMenu(&m);
  add_menu->setTitle(tr("Add"));
  connect(add_menu, &Menu::triggered, this, &NodeView::CreateNodeSlot);
  m.addMenu(add_menu);

  m.exec(mapToGlobal(pos));
}

void NodeView::CreateNodeSlot(QAction *action)
{
  Node* new_node = NodeFactory::CreateFromMenuAction(action);

  if (new_node) {
    Core::instance()->undo_stack()->push(new NodeAddCommand(graph_, new_node));

    NodeViewItem* item = scene_.NodeToUIObject(new_node);
    AttachItemToCursor(item);
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

    qDebug() << "Moving" << node_item->GetNode() << "for" << n->GetNode();

    QPointF new_pos;

    if (item->pos() == pos) {
      qDebug() << "Same pos, need more info";

      // Item positions are exact, we'll need more information to determine where this item should go
      Node* ours = n->GetNode();
      Node* theirs = node_item->GetNode();

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

void NodeView::AttachItemToCursor(NodeViewItem *item)
{
  attached_item_ = item;

  setMouseTracking(attached_item_);
}

void NodeView::DetachItemFromCursor()
{
  AttachItemToCursor(nullptr);
}

OLIVE_NAMESPACE_EXIT
