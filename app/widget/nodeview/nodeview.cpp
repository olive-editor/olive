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

#include "nodeview.h"

#include <QInputDialog>
#include <QMouseEvent>
#include <QScrollBar>

#include "core.h"
#include "nodeviewundo.h"
#include "node/factory.h"
#include "widget/menu/menushared.h"
#include "widget/timebased/timebasedview.h"

#define super HandMovableView

namespace olive {

const double NodeView::kMinimumScale = 0.1;

NodeView::NodeView(QWidget *parent) :
  HandMovableView(parent),
  graph_(nullptr),
  drop_edge_(nullptr),
  create_edge_(nullptr),
  create_edge_dst_(nullptr),
  create_edge_dst_temp_expanded_(false),
  filter_mode_(kFilterShowSelectedBlocks),
  scale_(1.0)
{
  setScene(&scene_);
  SetDefaultDragMode(RubberBandDrag);
  setContextMenuPolicy(Qt::CustomContextMenu);
  setMouseTracking(true);
  setRenderHint(QPainter::Antialiasing);
  setViewportUpdateMode(FullViewportUpdate);

  connect(this, &NodeView::customContextMenuRequested, this, &NodeView::ShowContextMenu);

  ConnectSelectionChangedSignal();

  SetFlowDirection(NodeViewCommon::kTopToBottom);

  // Set massive scene rect and hide the scrollbars to create an "infinite space" effect
  scene_.setSceneRect(-1000000, -1000000, 2000000, 2000000);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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

  if (graph_) {
    disconnect(graph_, &NodeGraph::NodeAdded, &scene_, &NodeViewScene::AddNode);
    disconnect(graph_, &NodeGraph::NodeRemoved, &scene_, &NodeViewScene::RemoveNode);
    disconnect(graph_, &NodeGraph::InputConnected, &scene_, &NodeViewScene::AddEdge);
    disconnect(graph_, &NodeGraph::InputDisconnected, &scene_, &NodeViewScene::RemoveEdge);

    DeselectAll();

    // Clear the scene of all UI objects
    scene_.clear();
  }

  // Set reference to the graph
  graph_ = graph;

  // If the graph is valid, add UI objects for each of its Nodes
  if (graph_) {
    connect(graph_, &NodeGraph::NodeAdded, &scene_, &NodeViewScene::AddNode);
    connect(graph_, &NodeGraph::NodeRemoved, &scene_, &NodeViewScene::RemoveNode);
    connect(graph_, &NodeGraph::InputConnected, &scene_, &NodeViewScene::AddEdge);
    connect(graph_, &NodeGraph::InputDisconnected, &scene_, &NodeViewScene::RemoveEdge);

    foreach (Node* n, graph_->nodes()) {
      scene_.AddNode(n);
    }

    foreach (Node* n, graph_->nodes()) {
      for (auto it=n->input_connections().cbegin(); it!=n->input_connections().cend(); it++) {
        scene_.AddEdge(it->second, it->first);
      }
    }
  }
}

void NodeView::DeleteSelected()
{
  if (!graph_) {
    return;
  }

  MultiUndoCommand* command = new MultiUndoCommand();

  {
    QVector<NodeViewEdge *> selected_edges = scene_.GetSelectedEdges();

    foreach (NodeViewEdge* edge, selected_edges) {
      command->add_child(new NodeEdgeRemoveCommand(edge->output(), edge->input()));
    }
  }

  {
    QVector<Node*> selected_nodes = scene_.GetSelectedNodes();

    // Ensure no nodes are "undeletable"
    for (int i=0;i<selected_nodes.size();i++) {
      if (!selected_nodes.at(i)->CanBeDeleted()) {
        selected_nodes.removeAt(i);
        i--;
      }
    }

    if (!selected_nodes.isEmpty()) {
      foreach (Node* node, selected_nodes) {
        command->add_child(new NodeRemoveAndDisconnectCommand(node));
      }
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void NodeView::SelectAll()
{
  if (!graph_) {
    return;
  }

  // Optimization: rather than respond to every single item being selected, ignore the signal and
  //               then handle them all at the end.
  DisconnectSelectionChangedSignal();

  scene_.SelectAll();

  ConnectSelectionChangedSignal();

  if (selected_nodes_.isEmpty()) {
    // No nodes were selected before so we can just emit them all
    emit NodesSelected(graph_->nodes());
  } else {
    // We have to determine the difference
    QVector<Node*> new_selection;
    foreach (Node* n, graph_->nodes()) {
      if (!selected_nodes_.contains(n)) {
        new_selection.append(n);
      }
    }

    emit NodesSelected(new_selection);
  }

  // Just add everything to the selected nodes list
  selected_nodes_ = graph_->nodes();
}

void NodeView::DeselectAll()
{
  if (!graph_ || selected_nodes_.isEmpty()) {
    return;
  }

  // Optimization: rather than respond to every single item being selected, ignore the signal and
  //               then handle them all at the end.
  DisconnectSelectionChangedSignal();

  scene_.DeselectAll();

  ConnectSelectionChangedSignal();

  // Just emit all the nodes that are currently selected as no longer selected
  emit NodesDeselected(selected_nodes_);
  selected_nodes_.clear();
}

void NodeView::Select(QVector<Node *> nodes)
{
  if (!graph_) {
    return;
  }

  // Optimization: rather than respond to every single item being selected, ignore the signal and
  //               then handle them all at the end.
  DisconnectSelectionChangedSignal();

  QVector<Node*> deselections = selected_nodes_;
  QVector<Node*> new_selections;

  scene_.DeselectAll();

  // Remove any duplicates
  QVector<Node*> processed;

  foreach (Node* n, nodes) {
    if (processed.contains(n)) {
      continue;
    }

    processed.append(n);

    NodeViewItem* item = scene_.NodeToUIObject(n);

    item->setSelected(true);

    if (deselections.contains(n)) {
      deselections.removeOne(n);
    } else {
      new_selections.append(n);
    }
  }

  // Center on something
  centerOn(scene_.NodeToUIObject(nodes.first()));

  ConnectSelectionChangedSignal();

  // Emit deselect signal for any nodes that weren't in the list
  if (!deselections.isEmpty()) {
    emit NodesDeselected(deselections);
  }

  // Emit select signal for any nodes that weren't in the list
  if (!new_selections.isEmpty()) {
    emit NodesSelected(new_selections);
  }

  // Update selected list to the list we received
  selected_nodes_ = nodes;
}

void NodeView::SelectWithDependencies(QVector<Node *> nodes)
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

  QVector<Node*> selected = scene_.GetSelectedNodes();

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

  MultiUndoCommand* command = new MultiUndoCommand();

  QVector<Node*> pasted_nodes = PasteNodesFromClipboard(graph_, command);

  Core::instance()->undo_stack()->pushIfHasChildren(command);

  if (!pasted_nodes.isEmpty()) {
    AttachNodesToCursor(pasted_nodes);
  }
}

void NodeView::Duplicate()
{
  if (!graph_) {
    return;
  }

  QVector<Node*> selected = scene_.GetSelectedNodes();

  if (selected.isEmpty()) {
    return;
  }

  MultiUndoCommand* command = new MultiUndoCommand();

  QVector<Node*> duplicated_nodes = Node::CopyDependencyGraph(selected, command);

  Core::instance()->undo_stack()->pushIfHasChildren(command);

  AttachNodesToCursor(duplicated_nodes);
}

void NodeView::SetColorLabel(int index)
{
  foreach (Node* node, selected_nodes_) {
    node->SetOverrideColor(index);
  }
}

void NodeView::ZoomIn()
{
  ZoomFromKeyboard(1.25);
}

void NodeView::ZoomOut()
{
  ZoomFromKeyboard(0.8);
}

void NodeView::keyPressEvent(QKeyEvent *event)
{
  super::keyPressEvent(event);

  if (event->key() == Qt::Key_Escape && !attached_items_.isEmpty()) {
    DetachItemsFromCursor();

    // We undo the last action which SHOULD be adding the node
    // FIXME: Possible danger of this not being the case?
    Core::instance()->undo_stack()->undo();
  }
}

void NodeView::mousePressEvent(QMouseEvent *event)
{
  if (HandPress(event)) return;

  if (event->button() == Qt::RightButton) {
    QGraphicsItem* item = itemAt(event->pos());

    if (!item || !item->isSelected()) {
      // Qt doesn't do this by default for some reason
      if (!(event->modifiers() & Qt::ShiftModifier)) {
        scene_.clearSelection();
      }

      // If there's an item here, select it
      if (item) {
        item->setSelected(true);
      }
    }
  }

  if (event->modifiers() & Qt::ControlModifier) {
    NodeViewItem* item = dynamic_cast<NodeViewItem*>(itemAt(event->pos()));

    if (item) {
      create_edge_ = new NodeViewEdge();
      create_edge_src_ = item;

      create_edge_->SetCurved(scene_.GetEdgesAreCurved());
      create_edge_->SetFlowDirection(scene_.GetFlowDirection());

      scene_.addItem(create_edge_);
      return;
    }
  }

  super::mousePressEvent(event);
}

void NodeView::mouseMoveEvent(QMouseEvent *event)
{
  if (HandMove(event)) return;

  if (create_edge_) {
    // Determine scene coordinate
    QPointF scene_pt = mapToScene(event->pos());

    // Find if the cursor is currently inside an item
    NodeViewItem* item_at_cursor = dynamic_cast<NodeViewItem*>(itemAt(event->pos()));

    // Filter out connecting to self1
    if (item_at_cursor == create_edge_src_) {
      item_at_cursor = nullptr;
    }

    // If the item has changed
    if (item_at_cursor != create_edge_dst_) {
      // If we had a destination active, disconnect from it since the item has changed
      if (create_edge_dst_) {
        create_edge_dst_->SetHighlightedIndex(-1);

        if (create_edge_dst_temp_expanded_) {
          // We expanded this item, so we can un-expand it
          create_edge_dst_->SetExpanded(false);
        }
      }

      // Set destination
      create_edge_dst_ = item_at_cursor;

      // If our destination is an item, ensure it's expanded
      if (create_edge_dst_) {
        if ((create_edge_dst_temp_expanded_ = (!create_edge_dst_->IsExpanded()))) {
          create_edge_dst_->SetExpanded(true, true);
        }
      }
    }

    // If we have a destination, highlight the appropriate input
    int highlight_index = -1;
    if (create_edge_dst_) {
      highlight_index = create_edge_dst_->GetIndexAt(scene_pt);
      create_edge_dst_->SetHighlightedIndex(highlight_index);
    }

    if (highlight_index >= 0) {
      create_edge_dst_input_ = create_edge_dst_->GetInputAtIndex(highlight_index);
      create_edge_->SetPoints(create_edge_src_->GetOutputPoint(Node::kDefaultOutput),
                              create_edge_dst_->GetInputPoint(create_edge_dst_input_.input(), create_edge_dst_input_.element(), create_edge_src_->pos()),
                              true);
    } else {
      create_edge_dst_input_.Reset();
      create_edge_->SetPoints(create_edge_src_->GetOutputPoint(Node::kDefaultOutput),
                              scene_pt,
                              false);
    }

    // Set connected to whether we have a valid input destination
    create_edge_->SetConnected(create_edge_dst_input_.IsValid());
    return;
  }

  super::mouseMoveEvent(event);

  // See if there are any items attached
  if (!attached_items_.isEmpty()) {
    // Move those items to the cursor
    MoveAttachedNodesToCursor(event->pos());

    // See if the user clicked on an edge (only when dropping single nodes)
    if (attached_items_.size() == 1) {
      Node* attached_node = attached_items_.first().item->GetNode();

      QRect edge_detect_rect(event->pos(), event->pos());

      int edge_detect_radius = fontMetrics().height();
      edge_detect_rect.adjust(-edge_detect_radius, -edge_detect_radius, edge_detect_radius, edge_detect_radius);

      QList<QGraphicsItem*> items = this->items(edge_detect_rect);

      NodeViewEdge* new_drop_edge = nullptr;

      // See if there is an edge here
      foreach (QGraphicsItem* item, items) {
        new_drop_edge = dynamic_cast<NodeViewEdge*>(item);

        if (new_drop_edge) {
          drop_input_.Reset();

          foreach (const QString& input, attached_node->inputs()) {
            NodeInput i(attached_node, input);

            if (attached_node->IsInputConnectable(input)) {
              if (attached_node->GetInputDataType(input) == new_drop_edge->input().GetDataType()) {
                drop_input_ = i;
                break;
              } else if (!drop_input_.IsValid()) {
                drop_input_ = i;
              }
            }
          }

          if (drop_input_.IsValid()) {
            break;
          } else {
            new_drop_edge = nullptr;
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
}

void NodeView::mouseReleaseEvent(QMouseEvent *event)
{
  if (HandRelease(event)) return;

  if (create_edge_) {
    delete create_edge_;
    create_edge_ = nullptr;

    if (create_edge_dst_) {
      // Clear highlight
      create_edge_dst_->SetHighlightedIndex(-1);

      // Collapse if we expanded it
      if (create_edge_dst_temp_expanded_) {
        create_edge_dst_->SetExpanded(false);
      }

      if (create_edge_dst_input_.IsValid()) {
        // Make connection
        Core::instance()->undo_stack()->push(new NodeEdgeAddCommand(create_edge_src_->GetNode(), create_edge_dst_input_));
        create_edge_dst_input_.Reset();
      }

      create_edge_dst_ = nullptr;
    }
    return;
  }

  if (!attached_items_.isEmpty()) {
    if (attached_items_.size() == 1) {
      Node* dropping_node = attached_items_.first().item->GetNode();

      if (drop_edge_) {
        // We have everything we need to place the node in between
        MultiUndoCommand* command = new MultiUndoCommand();

        // Remove old edge
        command->add_child(new NodeEdgeRemoveCommand(drop_edge_->output(), drop_edge_->input()));

        // Place new edges
        command->add_child(new NodeEdgeAddCommand(drop_edge_->output(), drop_input_));
        command->add_child(new NodeEdgeAddCommand(dropping_node, drop_edge_->input()));

        Core::instance()->undo_stack()->push(command);
      }

      drop_edge_ = nullptr;
    }

    DetachItemsFromCursor();
  }

  super::mouseReleaseEvent(event);
}

void NodeView::wheelEvent(QWheelEvent *event)
{
  if (TimeBasedView::WheelEventIsAZoomEvent(event)) {
    qreal multiplier = 1.0 + (static_cast<qreal>(event->angleDelta().x() + event->angleDelta().y()) * 0.001);

    QPointF cursor_pos;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    cursor_pos = event->position();
#else
    cursor_pos = event->posF();
#endif

    ZoomIntoCursorPosition(multiplier, cursor_pos);
  } else {
    QWidget::wheelEvent(event);
  }
}

void NodeView::UpdateSelectionCache()
{
  QVector<Node*> current_selection = scene_.GetSelectedNodes();

  QVector<Node*> selected;
  QVector<Node*> deselected;

  // Determine which nodes are newly selected
  if (selected_nodes_.isEmpty()) {
    // All nodes in the current selection have just been selected
    selected = current_selection;
  } else {
    foreach (Node* n, current_selection) {
      if (!selected_nodes_.contains(n)) {
        selected.append(n);
      }
    }
  }

  // Determine which nodes are newly deselected
  if (current_selection.isEmpty()) {
    // All nodes that were selected have been deselected
    deselected = selected_nodes_;
  } else {
    foreach (Node* n, selected_nodes_) {
      if (!current_selection.contains(n)) {
        deselected.append(n);
      }
    }
  }

  selected_nodes_ = current_selection;

  if (!selected.isEmpty()) {
    emit NodesSelected(selected);
  }

  if (!deselected.isEmpty()) {
    emit NodesDeselected(deselected);
  }
}

void NodeView::ShowContextMenu(const QPoint &pos)
{
  if (!graph_) {
    return;
  }

  Menu m;

  MenuShared::instance()->AddItemsForEditMenu(&m, false);

  m.addSeparator();

  QVector<NodeViewItem*> selected = scene_.GetSelectedItems();

  if (itemAt(pos) && !selected.isEmpty()) {

    // Label node action
    QAction* label_action = m.addAction(tr("Label"));
    connect(label_action, &QAction::triggered, this, [this](){
      Core::instance()->LabelNodes(scene_.GetSelectedNodes());
    });

    // Color menu
    MenuShared::instance()->AddColorCodingMenu(&m);

    m.addSeparator();

    // Auto-position action
    QAction* autopos = m.addAction(tr("Auto-Position"));
    connect(autopos, &QAction::triggered, this, &NodeView::AutoPositionDescendents);

  } else {

    QAction* curved_action = m.addAction(tr("Smooth Edges"));
    curved_action->setCheckable(true);
    curved_action->setChecked(scene_.GetEdgesAreCurved());
    connect(curved_action, &QAction::triggered, &scene_, &NodeViewScene::SetEdgesAreCurved);

    m.addSeparator();

    Menu* filter_menu = new Menu(tr("Filter"), &m);
    m.addMenu(filter_menu);

    filter_menu->AddActionWithData(tr("Show All"),
                                   kFilterShowAll,
                                   filter_mode_);

    filter_menu->AddActionWithData(tr("Show Selected Blocks Only"),
                                   kFilterShowSelectedBlocks,
                                   filter_mode_);

    connect(filter_menu, &Menu::triggered, this, &NodeView::ContextMenuFilterChanged);



    Menu* direction_menu = new Menu(tr("Direction"), &m);
    m.addMenu(direction_menu);

    direction_menu->AddActionWithData(tr("Top to Bottom"),
                                      NodeViewCommon::kTopToBottom,
                                      scene_.GetFlowDirection());

    direction_menu->AddActionWithData(tr("Bottom to Top"),
                                      NodeViewCommon::kBottomToTop,
                                      scene_.GetFlowDirection());

    direction_menu->AddActionWithData(tr("Left to Right"),
                                      NodeViewCommon::kLeftToRight,
                                      scene_.GetFlowDirection());

    direction_menu->AddActionWithData(tr("Right to Left"),
                                      NodeViewCommon::kRightToLeft,
                                      scene_.GetFlowDirection());

    connect(direction_menu, &Menu::triggered, this, &NodeView::ContextMenuSetDirection);

    m.addSeparator();

    Menu* add_menu = NodeFactory::CreateMenu(&m);
    add_menu->setTitle(tr("Add"));
    connect(add_menu, &Menu::triggered, this, &NodeView::CreateNodeSlot);
    m.addMenu(add_menu);

  }

  m.exec(mapToGlobal(pos));
}

void NodeView::CreateNodeSlot(QAction *action)
{
  Node* new_node = NodeFactory::CreateFromMenuAction(action);

  if (new_node) {
    Core::instance()->undo_stack()->push(new NodeAddCommand(graph_, new_node));

    NodeViewItem* item = scene_.NodeToUIObject(new_node);
    AttachItemsToCursor({item});
  }
}

void NodeView::ContextMenuSetDirection(QAction *action)
{
  SetFlowDirection(static_cast<NodeViewCommon::FlowDirection>(action->data().toInt()));
}

void NodeView::AutoPositionDescendents()
{
  QVector<Node*> selected = scene_.GetSelectedNodes();

  foreach (Node* n, selected) {
    scene_.ReorganizeFrom(n);
  }
}

void NodeView::ContextMenuFilterChanged(QAction *action)
{
  Q_UNUSED(action)
}

void NodeView::AttachNodesToCursor(const QVector<Node *> &nodes)
{
  QVector<NodeViewItem*> items(nodes.size());

  for (int i=0; i<nodes.size(); i++) {
    items[i] = scene_.NodeToUIObject(nodes.at(i));
  }

  AttachItemsToCursor(items);
}

void NodeView::AttachItemsToCursor(const QVector<NodeViewItem*>& items)
{
  DetachItemsFromCursor();

  if (!items.isEmpty()) {
    foreach (NodeViewItem* i, items) {
      attached_items_.append({i, i->pos() - items.first()->pos()});
    }

    setMouseTracking(true);

    MoveAttachedNodesToCursor(mapFromGlobal(QCursor::pos()));
  }
}

void NodeView::DetachItemsFromCursor()
{
  attached_items_.clear();
  setMouseTracking(false);
}

void NodeView::SetFlowDirection(NodeViewCommon::FlowDirection dir)
{
  scene_.SetFlowDirection(dir);
}

void NodeView::MoveAttachedNodesToCursor(const QPoint& p)
{
  QPointF item_pos = mapToScene(p);

  foreach (const AttachedItem& i, attached_items_) {
    i.item->setPos(item_pos + i.original_pos);
  }
}

void NodeView::ConnectSelectionChangedSignal()
{
  connect(&scene_, &QGraphicsScene::selectionChanged, this, &NodeView::UpdateSelectionCache);
}

void NodeView::DisconnectSelectionChangedSignal()
{
  disconnect(&scene_, &QGraphicsScene::selectionChanged, this, &NodeView::UpdateSelectionCache);
}

void NodeView::ZoomIntoCursorPosition(double multiplier, const QPointF& cursor_pos)
{
  double test_scale = scale_ * multiplier;

  if (test_scale > kMinimumScale) {
    int anchor_x = qRound(double(cursor_pos.x() + horizontalScrollBar()->value()) / scale_ * test_scale - cursor_pos.x());
    int anchor_y = qRound(double(cursor_pos.y() + verticalScrollBar()->value()) / scale_ * test_scale - cursor_pos.y());

    scale(multiplier, multiplier);

    this->horizontalScrollBar()->setValue(anchor_x);
    this->verticalScrollBar()->setValue(anchor_y);

    scale_ = test_scale;
  }
}

void NodeView::ZoomFromKeyboard(double multiplier)
{
  QPoint cursor_pos = mapFromGlobal(QCursor::pos());

  // If the cursor is not currently within the widget, zoom into the center
  if (!rect().contains(cursor_pos)) {
    cursor_pos = QPoint(width()/2, height()/2);
  }

  ZoomIntoCursorPosition(multiplier, cursor_pos);
}

}
