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

#include "nodeview.h"

#include <cfloat>
#include <QInputDialog>
#include <QMouseEvent>
#include <QScrollBar>

#include "core.h"
#include "nodeviewundo.h"
#include "node/factory.h"
#include "node/traverser.h"
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
  paste_command_(nullptr),
  filter_mode_(kFilterShowSelective),
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
}

NodeView::~NodeView()
{
  // Unset the current graph
  ClearGraph();
}

void NodeView::SetGraph(NodeGraph *graph, const QVector<Node*> &nodes)
{
  bool graph_changed = graph_ != graph;
  bool context_changed = filter_nodes_ != nodes;

  if (graph_changed || context_changed) {
    // Clear nodes if necessary
    bool refresh_required = (graph_changed && filter_mode_ == kFilterShowAll)
        || (context_changed && filter_mode_ == kFilterShowSelective);
    bool nodes_visible = (graph && filter_mode_ == kFilterShowAll)
        || (!nodes.isEmpty() && filter_mode_ == kFilterShowSelective);

    if (refresh_required) {
      DeselectAll();
      positions_.clear();
      scene_.clear();
    }

    // Handle graph change
    if (graph_changed) {
      if (graph_) {
        // Disconnect from current graph
        disconnect(graph_, &NodeGraph::NodeAdded, this, &NodeView::AddNode);
        disconnect(graph_, &NodeGraph::NodeRemoved, this, &NodeView::RemoveNode);
        disconnect(graph_, &NodeGraph::InputConnected, this, &NodeView::AddEdge);
        disconnect(graph_, &NodeGraph::InputDisconnected, this, &NodeView::RemoveEdge);
        disconnect(graph_, &NodeGraph::NodePositionAdded, this, &NodeView::AddNodePosition);
        disconnect(graph_, &NodeGraph::NodePositionRemoved, this, &NodeView::RemoveNodePosition);
      }

      graph_ = graph;

      if (graph_) {
        // Connect to new graph
        connect(graph_, &NodeGraph::NodeAdded, this, &NodeView::AddNode);
        connect(graph_, &NodeGraph::NodeRemoved, this, &NodeView::RemoveNode);
        connect(graph_, &NodeGraph::InputConnected, this, &NodeView::AddEdge);
        connect(graph_, &NodeGraph::InputDisconnected, this, &NodeView::RemoveEdge);
        connect(graph_, &NodeGraph::NodePositionAdded, this, &NodeView::AddNodePosition);
        connect(graph_, &NodeGraph::NodePositionRemoved, this, &NodeView::RemoveNodePosition);
      }
    }

    if (context_changed) {
      filter_nodes_ = nodes;
    }

    if (refresh_required && nodes_visible) {

      if (filter_mode_ == kFilterShowAll) {
        // FIXME: Implement
      } else {
        // Reserve an arbitrary number to reduce the amount of reallocations
        qreal last_offset = 0;
        int additional_spacing = 0;

        foreach (Node *n, filter_nodes_) {
          const NodeGraph::PositionMap &map = graph_->GetNodesForContext(n);

          // First determine the total "height" of this graph and how much we need to offset it
          qreal top = 0;
          qreal bottom = 0;
          for (auto it=map.cbegin(); it!=map.cend(); it++) {
            const QPointF &node_pos_in_context = it.value();
            top = qMin(node_pos_in_context.y(), top);
            bottom = qMax(node_pos_in_context.y(), bottom);
          }

          last_offset += (additional_spacing + (bottom - top));
          additional_spacing = 1;
          context_offsets_.insert(n, QPointF(0, last_offset));

          // Finally add all nodes
          for (auto it=map.cbegin(); it!=map.cend(); it++) {
            AddNodePosition(it.key(), n);
          }
        }
      }
    }
  }
}

void NodeView::ClearGraph()
{
  SetGraph(nullptr, QVector<Node*>());
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

  NodeViewItem *first_item = nullptr;

  foreach (Node* n, nodes) {
    if (processed.contains(n)) {
      continue;
    }

    processed.append(n);

    NodeViewItem* item = scene_.NodeToUIObject(n);

    if (item) {
      item->setSelected(true);

      if (!first_item) {
        first_item = item;
      }

      if (deselections.contains(n)) {
        deselections.removeOne(n);
      } else {
        new_selections.append(n);
      }
    }
  }

  // Center on something
  if (first_item) {
    centerOn(first_item);
  }

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

  paste_command_ = new MultiUndoCommand();

  QVector<Node*> pasted_nodes = PasteNodesFromClipboard(graph_, paste_command_);

  if (!pasted_nodes.isEmpty()) {
    paste_command_->add_child(new NodeViewAttachNodesToCursor(this, pasted_nodes));
  }

  paste_command_->redo();
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

  paste_command_ = new MultiUndoCommand();

  QVector<Node*> duplicated_nodes = Node::CopyDependencyGraph(selected, paste_command_);

  if (!duplicated_nodes.isEmpty()) {
    paste_command_->add_child(new NodeViewAttachNodesToCursor(this, duplicated_nodes));
  }

  paste_command_->redo();
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
    if (paste_command_) {
      paste_command_->undo();
      delete paste_command_;
      paste_command_ = nullptr;
    }
  }
}

void NodeView::mousePressEvent(QMouseEvent *event)
{
  if (HandPress(event)) return;

  QGraphicsItem* item = itemAt(event->pos());

  if (event->button() == Qt::LeftButton) {
    NodeViewEdge* edge_item = dynamic_cast<NodeViewEdge*>(item);
    if (edge_item && edge_item->arrow_bounding_rect().contains(mapToScene(event->pos()))) {
      create_edge_src_ = scene_.NodeToUIObject(edge_item->output().node());
      create_edge_src_output_ = edge_item->output().output();
      create_edge_ = edge_item;
      create_edge_already_exists_ = true;
      return;
    }
  }

  if (event->button() == Qt::RightButton) {
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
    NodeViewItem* node_item = dynamic_cast<NodeViewItem*>(item);
    if (node_item) {
      create_edge_ = new NodeViewEdge();
      create_edge_src_ = node_item;
      create_edge_src_output_ = Node::kDefaultOutput;
      create_edge_already_exists_ = false;

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

    // Filter out connecting to self
    if (item_at_cursor == create_edge_src_) {
      item_at_cursor = nullptr;
    }

    // Filter out connecting to a node that connects to us
    if (item_at_cursor && item_at_cursor->GetNode()->OutputsTo(create_edge_src_->GetNode(), true)) {
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
          create_edge_dst_->setZValue(0);
        }
      }

      // Set destination
      create_edge_dst_ = item_at_cursor;

      // If our destination is an item, ensure it's expanded
      if (create_edge_dst_) {
        if ((create_edge_dst_temp_expanded_ = (!create_edge_dst_->IsExpanded()))) {
          create_edge_dst_->SetExpanded(true, true);
          create_edge_dst_->setZValue(100); // Ensure item is in front
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

          NodeValue::Type drop_edge_data_type = NodeValue::kNone;

          // Run the Node and guess what type it's actually returning
          NodeTraverser traverser;
          NodeValueTable table = traverser.GenerateTable(new_drop_edge->output(), TimeRange(0, 0));
          if (table.Count() > 0) {
            drop_edge_data_type = table.at(table.Count() - 1).type();
          }

          // Iterate through the inputs of our dragging node and see if our node has any acceptable
          // inputs to connect to for this type
          foreach (const QString& input, attached_node->inputs()) {
            NodeInput i(attached_node, input);

            if (attached_node->IsInputConnectable(input)) {
              if (attached_node->GetInputDataType(input) == drop_edge_data_type) {
                // Found exactly the type we're looking for, set and break this loop
                drop_input_ = i;
                break;
              } else if (!drop_input_.IsValid()) {
                // Default to first connectable input
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
    MultiUndoCommand* command = new MultiUndoCommand();

    if (create_edge_already_exists_) {
      if (!create_edge_->IsConnected()) {
        command->add_child(new NodeEdgeRemoveCommand(create_edge_->output(), create_edge_->input()));
      }
    } else {
      delete create_edge_;
    }

    create_edge_ = nullptr;

    if (create_edge_dst_) {
      // Clear highlight
      create_edge_dst_->SetHighlightedIndex(-1);

      // Collapse if we expanded it
      if (create_edge_dst_temp_expanded_) {
        create_edge_dst_->SetExpanded(false);
        create_edge_dst_->setZValue(0);
      }

      if (create_edge_dst_input_.IsValid()) {
        // Make connection
        command->add_child(new NodeEdgeAddCommand(NodeOutput(create_edge_src_->GetNode(), create_edge_src_output_), create_edge_dst_input_));
        create_edge_dst_input_.Reset();
      }

      create_edge_dst_ = nullptr;
    }

    Core::instance()->undo_stack()->pushIfHasChildren(command);
    return;
  }

  MultiUndoCommand* command = new MultiUndoCommand();

  if (!attached_items_.isEmpty()) {
    if (paste_command_) {
      // We've already "done" this command, but MultiUndoCommand prevents "redoing" twice, so we
      // add it to this command (which may have extra commands added too) so that it all gets undone
      // in the same action
      command->add_child(paste_command_);
      paste_command_ = nullptr;
    }

    if (attached_items_.size() == 1) {
      Node* dropping_node = attached_items_.first().item->GetNode();

      if (drop_edge_) {
        // Remove old edge
        command->add_child(new NodeEdgeRemoveCommand(drop_edge_->output(), drop_edge_->input()));

        // Place new edges
        command->add_child(new NodeEdgeAddCommand(drop_edge_->output(), drop_input_));
        command->add_child(new NodeEdgeAddCommand(dropping_node, drop_edge_->input()));
      }

      drop_edge_ = nullptr;
    }

    DetachItemsFromCursor();
  }

  for (auto it=positions_.begin(); it!=positions_.end(); it++) {
    NodeViewItem *item = it.key();
    Position &pos_data = it.value();
    QPointF current_item_pos = item->GetNodePosition();
    Node *node = pos_data.node;

    if (pos_data.original_item_pos != current_item_pos) {
      QPointF diff = current_item_pos - pos_data.original_item_pos;

      foreach (Node *context, filter_nodes_) {
        if (graph_->ContextContainsNode(node, context)) {
          QPointF current_node_pos_in_context = graph_->GetNodePosition(node, context);
          current_node_pos_in_context += diff;
          command->add_child(new NodeSetPositionCommand(node, context, current_node_pos_in_context, false));
        }
      }

      pos_data.original_item_pos = current_item_pos;
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);

  super::mouseReleaseEvent(event);
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

    ViewerOutput* viewer = dynamic_cast<ViewerOutput*>(selected.first()->GetNode());
    if (viewer) {
      m.addSeparator();
      QAction* open_in_viewer_action = m.addAction(tr("Open in Viewer"));
      connect(open_in_viewer_action, &QAction::triggered, this, &NodeView::OpenSelectedNodeInViewer);
    }

  } else {

    QAction* curved_action = m.addAction(tr("Smooth Edges"));
    curved_action->setCheckable(true);
    curved_action->setChecked(scene_.GetEdgesAreCurved());
    connect(curved_action, &QAction::triggered, &scene_, &NodeViewScene::SetEdgesAreCurved);

    m.addSeparator();

    AddSetScrollZoomsByDefaultActionToMenu(&m);

    m.addSeparator();

    Menu* filter_menu = new Menu(tr("Filter"), &m);
    m.addMenu(filter_menu);

    filter_menu->AddActionWithData(tr("Show All Nodes"),
                                   kFilterShowAll,
                                   filter_mode_);

    filter_menu->AddActionWithData(tr("Show Selected"),
                                   kFilterShowSelective,
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
    paste_command_ = new MultiUndoCommand();
    paste_command_->add_child(new NodeAddCommand(graph_, new_node));
    foreach (Node *context, filter_nodes_) {
      paste_command_->add_child(new NodeSetPositionCommand(new_node, context, QPointF(0, 0), false));
    }
    paste_command_->add_child(new NodeViewAttachNodesToCursor(this, {new_node}));
    paste_command_->redo();
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
  FilterMode mode = static_cast<FilterMode>(action->data().toInt());

  if (filter_mode_ != mode) {
    // Store temporary graph variables
    NodeGraph *graph = graph_;
    QVector<Node*> nodes = filter_nodes_;

    // Unset graph with current filter mode
    ClearGraph();

    // Change filter mode
    filter_mode_ = mode;

    // Re-set graph with new filter mode
    SetGraph(graph, nodes);
  }
}

void NodeView::OpenSelectedNodeInViewer()
{
  QVector<Node*> selected = scene_.GetSelectedNodes();
  ViewerOutput* viewer = selected.isEmpty() ? nullptr : dynamic_cast<ViewerOutput*>(selected.first());

  if (viewer) {
    Core::instance()->OpenNodeInViewer(viewer);
  }
}

void NodeView::AddNode(Node *node)
{
  if (filter_mode_ == kFilterShowAll) {
    scene_.AddNode(node);
  }
}

void NodeView::RemoveNode(Node *node)
{
  scene_.RemoveNode(node);
}

void NodeView::AddEdge(const NodeOutput &output, const NodeInput &input)
{
  if (filter_mode_ == kFilterShowAll) {
    scene_.AddEdge(output, input);
  } else if (filter_mode_ == kFilterShowSelective) {
    Node *output_node = output.node();
    Node *input_node = input.node();

    if (scene_.item_map().contains(output_node) && scene_.item_map().contains(input_node)) {
      scene_.AddEdge(output, input);
    }
  }
}

void NodeView::RemoveEdge(const NodeOutput &output, const NodeInput &input)
{
  scene_.RemoveEdge(output, input);
}

void NodeView::AddNodePosition(Node *node, Node *relative)
{
  if (filter_mode_ == kFilterShowSelective) {
    if (filter_nodes_.contains(relative)) {
      // Get UI item or create if it doesn't exist
      NodeViewItem *item = scene_.item_map().value(node);
      if (!item) {
        item = scene_.AddNode(node);

        for (auto it=node->input_connections().cbegin(); it!=node->input_connections().cend(); it++) {
          if (scene_.item_map().contains(it->second.node())) {
            scene_.AddEdge(it->second, it->first);
          }
        }

        for (auto it=node->output_connections().cbegin(); it!=node->output_connections().cend(); it++) {
          if (scene_.item_map().contains(it->second.node())) {
            scene_.AddEdge(it->first, it->second);
          }
        }
      }

      // Determine "view" position by averaging the Y value and "min"ing the X value of all contexts
      QPointF item_pos(DBL_MAX, 0.0);
      int average_count = 0;
      foreach (Node *context, filter_nodes_) {
        if (graph_->GetNodesForContext(context).contains(node)) {
          QPointF this_context_pos = graph_->GetNodePosition(node, context);
          this_context_pos += context_offsets_.value(context);
          item_pos.setX(qMin(item_pos.x(), this_context_pos.x()));
          item_pos.setY(item_pos.y() + this_context_pos.y());
          average_count++;
        }
      }
      item_pos.setY(item_pos.y() / average_count);

      // Set position
      item->SetNodePosition(item_pos);
      positions_.insert(item, {node, item_pos});
    }
  }
}

void NodeView::RemoveNodePosition(Node *node, Node *relative)
{
  if (filter_mode_ == kFilterShowSelective) {
    if (filter_nodes_.contains(relative)) {
      // Determine if any other contexts have this node
      bool found = false;

      foreach (Node *context, filter_nodes_) {
        if (graph_->ContextContainsNode(node, context)) {
          found = true;
          break;
        }
      }

      if (!found) {
        scene_.RemoveNode(node);
      }
    }
  }
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

    MoveAttachedNodesToCursor(mapFromGlobal(QCursor::pos()));
  }
}

void NodeView::DetachItemsFromCursor()
{
  attached_items_.clear();
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

void NodeView::ZoomIntoCursorPosition(QWheelEvent *event, double multiplier, const QPointF& cursor_pos)
{
  Q_UNUSED(event)

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

  ZoomIntoCursorPosition(nullptr, multiplier, cursor_pos);
}

NodeView::NodeViewAttachNodesToCursor::NodeViewAttachNodesToCursor(NodeView *view, const QVector<Node *> &nodes) :
  view_(view),
  nodes_(nodes)
{
}

void NodeView::NodeViewAttachNodesToCursor::redo()
{
  view_->AttachNodesToCursor(nodes_);
}

void NodeView::NodeViewAttachNodesToCursor::undo()
{
  view_->DetachItemsFromCursor();
}

Project *NodeView::NodeViewAttachNodesToCursor::GetRelevantProject() const
{
  // Will either return a project or a nullptr which is also acceptable
  return dynamic_cast<Project*>(view_->graph_);
}

}
