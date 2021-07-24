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
#include "node/audio/volume/volume.h"
#include "node/distort/transform/transformdistortnode.h"
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
  scale_(1.0),
  queue_reposition_contexts_(false)
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

  UpdateSceneBoundingRect();
  connect(&scene_, &QGraphicsScene::changed, this, &NodeView::UpdateSceneBoundingRect);

  minimap_ = new NodeViewMiniMap(&scene_, this);
  minimap_->show();
  connect(minimap_, &NodeViewMiniMap::Resized, this, &NodeView::RepositionMiniMap);
  connect(minimap_, &NodeViewMiniMap::MoveToScenePoint, this, &NodeView::MoveToScenePoint);
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &NodeView::UpdateViewportOnMiniMap);
  connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &NodeView::UpdateViewportOnMiniMap);

  viewport()->installEventFilter(this);
}

NodeView::~NodeView()
{
  // Unset the current graph
  ClearGraph();
}

void NodeView::SetGraph(NodeGraph *graph, const QVector<Node*> &nodes)
{
  bool graph_changed = graph_ != graph;
  bool context_changed = last_set_filter_nodes_ != nodes;

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
      context_offsets_.clear();
    }

    // Handle graph change
    if (graph_changed) {
      if (graph_) {
        // Disconnect from current graph
        disconnect(graph_, &NodeGraph::NodeRemoved, this, &NodeView::RemoveNode);
        disconnect(graph_, &NodeGraph::InputConnected, this, &NodeView::AddEdge);
        disconnect(graph_, &NodeGraph::InputDisconnected, this, &NodeView::RemoveEdge);
        disconnect(graph_, &NodeGraph::NodePositionAdded, this, &NodeView::AddNodePosition);
        disconnect(graph_, &NodeGraph::NodePositionRemoved, this, &NodeView::RemoveNodePosition);
      }

      graph_ = graph;

      if (graph_) {
        // Connect to new graph
        connect(graph_, &NodeGraph::NodeRemoved, this, &NodeView::RemoveNode);
        connect(graph_, &NodeGraph::InputConnected, this, &NodeView::AddEdge);
        connect(graph_, &NodeGraph::InputDisconnected, this, &NodeView::RemoveEdge);
        connect(graph_, &NodeGraph::NodePositionAdded, this, &NodeView::AddNodePosition);
        connect(graph_, &NodeGraph::NodePositionRemoved, this, &NodeView::RemoveNodePosition);
      }
    }

    if (context_changed) {
      last_set_filter_nodes_ = nodes;

      if (filter_mode_ == kFilterShowSelective) {
        filter_nodes_ = nodes;
      }
    }

    if (refresh_required && nodes_visible) {
      if (filter_mode_ == kFilterShowAll) {
        // Just make the filter nodes all of the graph's contexts
        filter_nodes_ = graph->GetPositionMap().keys().toVector();
      }

      RepositionContexts();

      // Center on something
      QMetaObject::invokeMethod(this, &NodeView::CenterOnItemsBoundingRect, Qt::QueuedConnection);
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
    // First remove any selected edges
    QVector<NodeViewEdge *> selected_edges = scene_.GetSelectedEdges();

    if (!selected_edges.isEmpty()) {
      Node::OutputConnections removed_connections(selected_edges.size());

      for (int i=0; i<selected_edges.size(); i++) {
        NodeViewEdge* edge = selected_edges.at(i);
        command->add_child(new NodeEdgeRemoveCommand(edge->output(), edge->input()));
        removed_connections[i] = {edge->output(), edge->input()};
      }

      // Update contexts
      UpdateContextsFromEdgeRemove(command, removed_connections);
    }
  }

  {
    // Secondly remove any nodes
    QVector<Node*> selected_nodes = scene_.GetSelectedNodes();

    // Ensure no nodes are "undeletable"
    for (int i=0;i<selected_nodes.size();i++) {
      if (!selected_nodes.at(i)->CanBeDeleted()) {
        selected_nodes.removeAt(i);
        i--;
      }
    }

    if (!selected_nodes.isEmpty()) {
      for (Node* node : qAsConst(selected_nodes)) {
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
    for (Node* n : graph_->nodes()) {
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

void NodeView::Select(QVector<Node *> nodes, bool center_view_on_item)
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

  for (Node* n : qAsConst(nodes)) {
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
  if (center_view_on_item && first_item) {
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

void NodeView::SelectWithDependencies(QVector<Node *> nodes, bool center_view_on_item)
{
  if (!graph_) {
    return;
  }

  int original_length = nodes.size();
  for (int i=0;i<original_length;i++) {
    nodes.append(nodes.at(i)->GetDependencies());
  }

  Select(nodes, center_view_on_item);
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
  PasteNodesInternal();
}

void NodeView::Duplicate()
{
  PasteNodesInternal(scene_.GetSelectedNodes());
}

void NodeView::SetColorLabel(int index)
{
  for (Node* node : qAsConst(selected_nodes_)) {
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
  switch (event->key()) {
  case Qt::Key_Left:
  case Qt::Key_Right:
  case Qt::Key_Up:
  case Qt::Key_Down:
  {
    if (graph_) {
      MultiUndoCommand *pos_command = new MultiUndoCommand();
      for (Node *n : qAsConst(selected_nodes_)) {
        for (Node *context : qAsConst(filter_nodes_)) {
          if (graph_->GetNodesForContext(context).contains(n)) {
            QPointF old_pos = graph_->GetNodePosition(n, context);

            // Determine one pixel in scene units
            double movement_amt = 1.0 / scale_;

            // Translate to 2D movement
            QPointF node_movement;
            switch (event->key()) {
            case Qt::Key_Left:
              node_movement.setX(-movement_amt);
              break;
            case Qt::Key_Right:
              node_movement.setX(movement_amt);
              break;
            case Qt::Key_Up:
              node_movement.setY(-movement_amt);
              break;
            case Qt::Key_Down:
              node_movement.setY(movement_amt);
              break;
            }

            // Translate from screen units into node units
            node_movement = NodeViewItem::ScreenToNodePoint(node_movement, scene_.GetFlowDirection());

            // Move command
            pos_command->add_child(new NodeSetPositionCommand(n, context, old_pos + node_movement, false));
          }
        }
      }
      Core::instance()->undo_stack()->pushIfHasChildren(pos_command);
    }
    break;
  }
  case Qt::Key_Escape:
    if (!attached_items_.isEmpty()) {
      DetachItemsFromCursor();

      // We undo the last action which SHOULD be adding the node
      if (paste_command_) {
        paste_command_->undo();
        delete paste_command_;
        paste_command_ = nullptr;
      }

      break;
    }

    /* fall through */
  default:
    super::keyPressEvent(event);
    break;
  }
}

void NodeView::mousePressEvent(QMouseEvent *event)
{
  if (HandPress(event)) return;

  if (event->button() == Qt::LeftButton) {
    // See if we're dragging the arrow of an edge
    QPointF scene_pt = mapToScene(event->pos());

    for (NodeViewEdge *edge_item : scene_.edges()) {
      if (edge_item->arrow_bounding_rect().contains(scene_pt)) {
        create_edge_src_ = scene_.NodeToUIObject(edge_item->output().node());
        create_edge_src_output_ = edge_item->output().output();
        create_edge_ = edge_item;
        create_edge_already_exists_ = true;
        return;
      }
    }

    // See if we're dragging the arrow of a node
    for (NodeViewItem *node_item : scene_.item_map()) {
      if (node_item->GetOutputTriangle().boundingRect().translated(node_item->pos()).contains(scene_pt)) {
        CreateNewEdge(node_item);
        return;
      }
    }
  }

  QGraphicsItem* item = itemAt(event->pos());

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
      CreateNewEdge(node_item);
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
      for (QGraphicsItem* item : qAsConst(items)) {
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
          for (const QString& input : attached_node->inputs()) {
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
    // We are creating a new edge or moving an existing one
    MultiUndoCommand* command = new MultiUndoCommand();

    Node::OutputConnections removed_edges;
    Node::OutputConnection added_edge;

    bool reconnected_to_itself = false;

    if (create_edge_already_exists_) {
      if (create_edge_dst_input_ == create_edge_->input()) {
        reconnected_to_itself = true;
      } else {
        // We are moving (or removing) an existing edge
        command->add_child(new NodeEdgeRemoveCommand(create_edge_->output(), create_edge_->input()));

        // Update contexts for edge removal
        removed_edges.push_back({create_edge_->output(), create_edge_->input()});
      }
    } else {
      // We're creating a new edge, which means this UI object is only temporary
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

      NodeInput &creating_input = create_edge_dst_input_;
      if (creating_input.IsValid()) {
        // Make connection
        if (!reconnected_to_itself) {
          NodeOutput creating_output(create_edge_src_->GetNode(), create_edge_src_output_);

          if (creating_input.IsConnected()) {
            Node::OutputConnection existing_edge_to_remove = {creating_input.GetConnectedOutput(), creating_input};
            command->add_child(new NodeEdgeRemoveCommand(existing_edge_to_remove.first, existing_edge_to_remove.second));
            removed_edges.push_back(existing_edge_to_remove);
          }

          command->add_child(new NodeEdgeAddCommand(creating_output, creating_input));
          added_edge = {creating_output, creating_input};
        }

        creating_input.Reset();
      }

      create_edge_dst_ = nullptr;
    }

    // Update contexts
    if (!removed_edges.empty()) {
      UpdateContextsFromEdgeRemove(command, removed_edges);
    }

    if (added_edge.first.IsValid()) {
      UpdateContextsFromEdgeAdd(command, added_edge, removed_edges);
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
  }

  {
    // If any node positions changed, set them in their contexts now
    MultiUndoCommand *set_pos_command = new MultiUndoCommand();
    for (auto it=positions_.begin(); it!=positions_.end(); it++) {
      NodeViewItem *item = it.key();
      Position &pos_data = it.value();
      QPointF current_item_pos = item->GetNodePosition();
      Node *node = pos_data.node;

      if (pos_data.original_item_pos != current_item_pos) {
        QPointF diff = current_item_pos - pos_data.original_item_pos;

        for (Node *context : qAsConst(filter_nodes_)) {
          if (graph_->ContextContainsNode(node, context)) {
            QPointF current_node_pos_in_context = graph_->GetNodePosition(node, context);
            current_node_pos_in_context += diff;
            set_pos_command->add_child(new NodeSetPositionCommand(node, context, current_node_pos_in_context, false));
          }
        }

        pos_data.original_item_pos = current_item_pos;
      }
    }
    if (set_pos_command->child_count()) {
      set_pos_command->redo();
      command->add_child(set_pos_command);
    } else {
      delete set_pos_command;
    }
  }


  if (!attached_items_.isEmpty()) {
    {
      // Dropped attached item onto an edge, connect it between them
      MultiUndoCommand *drop_edge_command = new MultiUndoCommand();
      if (attached_items_.size() == 1) {
        Node* dropping_node = attached_items_.first().item->GetNode();

        if (drop_edge_) {
          // Remove old edge
          drop_edge_command->add_child(new NodeEdgeRemoveCommand(drop_edge_->output(), drop_edge_->input()));

          // Place new edges
          drop_edge_command->add_child(new NodeEdgeAddCommand(drop_edge_->output(), drop_input_));
          drop_edge_command->add_child(new NodeEdgeAddCommand(dropping_node, drop_edge_->input()));
        }

        drop_edge_ = nullptr;
      }
      if (drop_edge_command->child_count()) {
        drop_edge_command->redo();
        command->add_child(drop_edge_command);
      } else {
        delete drop_edge_command;
      }
    }

    {
      // Remove from context any nodes that don't specifically output to said context
      MultiUndoCommand *remove_pos_command = new MultiUndoCommand();

      for (const AttachedItem &attached : qAsConst(attached_items_)) {
        MultiUndoCommand *remove_pos_subcommand = new MultiUndoCommand();
        Node *attached_node = scene_.item_map().key(attached.item);

        bool removed = false;
        QVector<Node*> relevant_contexts;
        for (Node *context : qAsConst(filter_nodes_)) {
          if (attached_node->OutputsTo(context, true)) {
            relevant_contexts.append(context);
          } else {
            remove_pos_subcommand->add_child(new NodeRemovePositionFromContextCommand(attached_node, context));
            removed = true;
          }
        }

        if (removed && !relevant_contexts.isEmpty()) {
          for (Node *relevant : qAsConst(relevant_contexts)) {
            remove_pos_subcommand->add_child(new NodeSetPositionCommand(attached_node, relevant, GetEstimatedPositionForContext(attached.item, relevant), false));
          }

          remove_pos_command->add_child(remove_pos_subcommand);
        } else {
          delete remove_pos_subcommand;
        }
      }

      if (remove_pos_command->child_count()) {
        remove_pos_command->redo();
        command->add_child(remove_pos_command);
      } else {
        delete remove_pos_command;
      }
    }

    DetachItemsFromCursor();
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);

  super::mouseReleaseEvent(event);
}

void NodeView::resizeEvent(QResizeEvent *event)
{
  super::resizeEvent(event);

  RepositionMiniMap();
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
    for (Node* n : qAsConst(current_selection)) {
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
    for (Node* n : qAsConst(selected_nodes_)) {
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

    filter_menu->AddActionWithData(tr("Show All Nodes"), kFilterShowAll, filter_mode_);

    filter_menu->AddActionWithData(tr("Show Selected"), kFilterShowSelective, filter_mode_);

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

    Menu* add_menu = CreateAddMenu(&m);
    m.addMenu(add_menu);

  }

  m.exec(mapToGlobal(pos));
}

void NodeView::CreateNodeSlot(QAction *action)
{
  if (!graph_) {
    return;
  }

  Node* new_node = NodeFactory::CreateFromMenuAction(action);

  if (new_node) {
    paste_command_ = new MultiUndoCommand();
    paste_command_->add_child(new NodeAddCommand(graph_, new_node));
    for (Node *context : qAsConst(filter_nodes_)) {
      paste_command_->add_child(new NodeSetPositionCommand(new_node, context, QPointF(0, 0), false));
    }
    paste_command_->add_child(new NodeViewAttachNodesToCursor(this, {new_node}));
    paste_command_->redo();

    this->setFocus();
  }
}

void NodeView::ContextMenuSetDirection(QAction *action)
{
  SetFlowDirection(static_cast<NodeViewCommon::FlowDirection>(action->data().toInt()));
}

void NodeView::ContextMenuFilterChanged(QAction *action)
{
  FilterMode mode = static_cast<FilterMode>(action->data().toInt());

  if (filter_mode_ != mode) {
    // Store temporary graph variables
    NodeGraph *graph = graph_;
    QVector<Node*> nodes = last_set_filter_nodes_;

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

// Commenting out because there shouldn't be any situations where a node would be added without
// being in a context. We're keeping RemoveNode as a fail-safe because it could provide crash
// resistance where RemoveNodePosition might be missed.
//void NodeView::AddNode(Node *node)
//{
//  if (filter_mode_ == kFilterShowAll) {
//    scene_.AddNode(node);
//  }
//}

void NodeView::RemoveNode(Node *node)
{
  scene_.RemoveNode(node);
}

void NodeView::AddEdge(const NodeOutput &output, const NodeInput &input)
{
  Node *output_node = output.node();
  Node *input_node = input.node();

  if (scene_.item_map().contains(output_node) && scene_.item_map().contains(input_node)) {
    scene_.AddEdge(output, input);
  }
}

void NodeView::RemoveEdge(const NodeOutput &output, const NodeInput &input)
{
  scene_.RemoveEdge(output, input);
}

void NodeView::AddNodePosition(Node *node, Node *relative)
{
  bool listening_to_node = filter_nodes_.contains(relative);

  if (!listening_to_node) {
    if (filter_mode_ == kFilterShowAll) {
      // We're not listening to this context, but because we're showing all, add it
      filter_nodes_.append(relative);
    } else {
      // Ignore signal
      return;
    }
  }

  // Reposition contexts because one of their heights may have changed or a new one may have been
  // added
  UpdateNodeItem(node);

  if (filter_mode_ == kFilterShowAll) {
    queue_reposition_contexts_ = true;
    viewport()->update();
  }
}

void NodeView::RemoveNodePosition(Node *node, Node *relative)
{
  if (filter_nodes_.contains(relative)) {
    NodeViewItem *item = scene_.item_map().value(node);

    if (item && !item->GetPreventRemoving()) {
      // Determine if any other contexts have this node
      bool found = false;

      for (Node *context : qAsConst(filter_nodes_)) {
        if (graph_->ContextContainsNode(node, context)) {
          found = true;
          break;
        }
      }

      if (!found) {
        for (const Node::OutputConnection &oc : node->output_connections()) {
          scene_.RemoveEdge(oc.first, oc.second);
        }
        for (auto it=node->input_connections().cbegin(); it!=node->input_connections().cend(); it++) {
          scene_.RemoveEdge(it->second, it->first);
        }
        positions_.remove(item);
        scene_.RemoveNode(node);
      }
    }

    if (filter_mode_ == kFilterShowAll) {
      RepositionContexts();
    }
  }
}

void NodeView::UpdateSceneBoundingRect()
{
  // Get current items bounding rect
  QRectF r = scene_.itemsBoundingRect();

  // Adjust so that it fills the view
  r.adjust(-width(), -height(), width(), height());

  // Set it
  scene_.setSceneRect(r);
}

void NodeView::CenterOnItemsBoundingRect()
{
  centerOn(scene_.itemsBoundingRect().center());
}

void NodeView::RepositionMiniMap()
{
  if (minimap_->isVisible()) {
    int margin = fontMetrics().height();

    int w = width() - minimap_->width() - margin;
    int h = height() - minimap_->height() - margin;

    if (verticalScrollBar()->isVisible()) {
      w -= verticalScrollBar()->width();
    }

    if (horizontalScrollBar()->isVisible()) {
      h -= horizontalScrollBar()->height();
    }

    minimap_->move(w, h);

    UpdateViewportOnMiniMap();
  }
}

void NodeView::UpdateViewportOnMiniMap()
{
  if (minimap_->isVisible()) {
    minimap_->SetViewportRect(mapToScene(viewport()->rect()));
  }
}

void NodeView::MoveToScenePoint(const QPointF &pos)
{
  centerOn(pos);
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
    for (NodeViewItem* i : items) {
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

  for (const AttachedItem& i : qAsConst(attached_items_)) {
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

bool NodeView::event(QEvent *event)
{
  if (event->type() == QEvent::ShortcutOverride) {
    QKeyEvent *se = static_cast<QKeyEvent *>(event);
    if (se->key() == Qt::Key_Left
        || se->key() == Qt::Key_Right
        || se->key() == Qt::Key_Up
        || se->key() == Qt::Key_Down) {
      se->accept();
      return true;
    }
  }

  return super::event(event);
}

bool NodeView::eventFilter(QObject *object, QEvent *event)
{
  if (object == viewport() && event->type() == QEvent::Paint) {
    if (queue_reposition_contexts_) {
      RepositionContexts();
      queue_reposition_contexts_ = false;
    }
  }

  return super::eventFilter(object, event);
}

void NodeView::CopyNodesToClipboardInternal(QXmlStreamWriter *writer, const QVector<Node *> &nodes, void *userdata)
{
  writer->writeStartElement(QStringLiteral("pos"));

  for (Node *n : nodes) {
    NodeViewItem *item = scene_.item_map().value(n);
    QPointF pos = item->GetNodePosition();

    writer->writeStartElement(QStringLiteral("node"));
    writer->writeAttribute(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(n)));
    writer->writeTextElement(QStringLiteral("x"), QString::number(pos.x()));
    writer->writeTextElement(QStringLiteral("y"), QString::number(pos.y()));
    writer->writeEndElement(); // node
  }

  writer->writeEndElement(); // pos
}

void NodeView::PasteNodesFromClipboardInternal(QXmlStreamReader *reader, XMLNodeData &xml_node_data, void *userdata)
{
  NodeGraph::PositionMap *map = static_cast<NodeGraph::PositionMap *>(userdata);

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("pos")) {
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("node")) {
          Node *n = nullptr;
          QPointF pos;

          XMLAttributeLoop(reader, attr) {
            if (attr.name() == QStringLiteral("ptr")) {
              n = xml_node_data.node_ptrs.value(attr.value().toULongLong());
              break;
            }
          }

          while (XMLReadNextStartElement(reader)) {
            if (reader->name() == QStringLiteral("x")) {
              pos.setX(reader->readElementText().toDouble());
            } else if (reader->name() == QStringLiteral("y")) {
              pos.setY(reader->readElementText().toDouble());
            } else {
              reader->skipCurrentElement();
            }
          }

          if (n) {
            map->insert(n, pos);
          }
        } else {
          reader->skipCurrentElement();
        }
      }
    } else {
      reader->skipCurrentElement();
    }
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

bool NodeView::DetermineIfNodeIsFloatingInContext(Node *node, Node *context, Node *source, const Node::OutputConnections &removed_edges, const Node::OutputConnection &added_edge)
{
  // Determines whether `node` outputs to another node in `context` besides `source`
  for (const Node::OutputConnection &conn : node->output_connections()) {
    Node *output_candidate = conn.second.node();

    if (output_candidate == source) {
      continue;
    }

    if (graph_->ContextContainsNode(output_candidate, context)) {
      if (!output_candidate->OutputsTo(source, true, removed_edges, added_edge)) {
        return true;
      }
    }
  }

  return false;
}

void NodeView::UpdateContextsFromEdgeRemove(MultiUndoCommand *command, const Node::OutputConnections &remove_edges)
{
  // For each edge we remove, determine if we should remove the node from a context as well
  for (const Node::OutputConnection &edge : remove_edges) {
    Node *output_node = edge.first.node();
    QVector<Node *> contexts_to_remove_from;
    int contexts_containing = 0;

    for (auto it=graph_->GetPositionMap().cbegin(); it!=graph_->GetPositionMap().cend(); it++) {
      Node *context = it.key();

      if (it.value().contains(output_node)) {
        bool currently_outputs = output_node->OutputsTo(context, true);
        bool will_output_after_operation = output_node->OutputsTo(context, true, remove_edges);

        if (currently_outputs && !will_output_after_operation) {
          // Will remove
          contexts_to_remove_from.append(context);
        }

        contexts_containing++;
      }
    }

    // Removing from all current contexts, convert to a floating node (i.e. don't remove from the context)
    if (contexts_to_remove_from.size() != contexts_containing) {
      // Not removing from all contexts, can remove
      bool removing_from_all_current_contexts = true;

      for (Node *context : qAsConst(filter_nodes_)) {
        if (graph_->ContextContainsNode(output_node, context)) {
          if (!contexts_to_remove_from.contains(context)) {
            removing_from_all_current_contexts = false;
            break;
          }
        }
      }

      for (Node *context : qAsConst(contexts_to_remove_from)) {
        RecursivelyRemoveFloatingNodeFromContext(command, output_node, context, output_node, remove_edges, Node::OutputConnection(), removing_from_all_current_contexts);
      }
    }
  }
}

void NodeView::RecursivelyRemoveFloatingNodeFromContext(MultiUndoCommand *command, Node *node, Node *context, Node *source, const Node::OutputConnections &removed_edges, const Node::OutputConnection &added_edge, bool prevent_removing)
{
  if (prevent_removing) {
    command->add_child(new NodeViewItemPreventRemovingCommand(this, node, true));
  }

  command->add_child(new NodeRemovePositionFromContextCommand(node, context));

  // Remove any dependency from the context that's also floating
  for (auto it=node->input_connections().cbegin(); it!=node->input_connections().cend(); it++) {
    Node *dependency = it->second.node();

    // Determine if this node happens to output to anything else in the context (which may be
    // another floating node that won't be removed by this operation)
    if (!DetermineIfNodeIsFloatingInContext(dependency, context, source, removed_edges, added_edge)) {
      RecursivelyRemoveFloatingNodeFromContext(command, dependency, context, source, removed_edges, added_edge, prevent_removing);
    }
  }
}

void NodeView::RecursivelyAddNodeToContext(MultiUndoCommand *command, Node *node, Node *context)
{
  command->add_child(new NodeSetPositionCommand(node, context, GetEstimatedPositionForContext(scene_.item_map().value(node), context), false));

  // Add dependency
  for (auto it=node->input_connections().cbegin(); it!=node->input_connections().cend(); it++) {
    Node *dependency = it->second.node();
    RecursivelyAddNodeToContext(command, dependency, context);
  }
}

void NodeView::UpdateContextsFromEdgeAdd(MultiUndoCommand *command, const Node::OutputConnection &added_edge, const Node::OutputConnections &removed_edges)
{
  // Determine if node currently does NOT output to a context that it WILL after this operation
  QVector<Node*> contexts_to_add_to;
  Node *connecting_node = added_edge.first.node();
  Node *input_node = added_edge.second.node();
  for (auto it=graph_->GetPositionMap().cbegin(); it!=graph_->GetPositionMap().cend(); it++) {
    if (it.value().contains(input_node)) {
      contexts_to_add_to.append(it.key());
    }
  }

  if (!contexts_to_add_to.isEmpty()) {
    // Determine whether the node is currently "floating", i.e. it outputs to none of the contexts
    // that it currently belongs to. If so, we will take ownership of it with this node.
    bool node_is_floating = true;
    QVector<Node*> current_contexts;
    for (auto it=graph_->GetPositionMap().cbegin(); it!=graph_->GetPositionMap().cend(); it++) {
      if (it.value().contains(connecting_node)) {
        if (connecting_node->OutputsTo(it.key(), true, removed_edges)) {
          node_is_floating = false;
          break;
        } else {
          current_contexts.append(it.key());
        }
      }
    }

    if (node_is_floating) {
      // This action will unfloat this node, so remove it from all current contexts
      for (Node *context : qAsConst(current_contexts)) {
        RecursivelyRemoveFloatingNodeFromContext(command, connecting_node, context, connecting_node, removed_edges, added_edge, false);
      }
    }

    // Add nodes to contexts
    for (Node *context : qAsConst(contexts_to_add_to)) {
      RecursivelyAddNodeToContext(command, connecting_node, context);
    }
  }
}

QPointF NodeView::GetEstimatedPositionForContext(NodeViewItem *item, Node *context) const
{
  return item->GetNodePosition() - context_offsets_.value(context);
}

Menu *NodeView::CreateAddMenu(Menu *parent)
{
  Menu* add_menu = NodeFactory::CreateMenu(parent);
  add_menu->setTitle(tr("Add"));
  connect(add_menu, &Menu::triggered, this, &NodeView::CreateNodeSlot);
  return add_menu;
}

void NodeView::CreateNewEdge(NodeViewItem *output_item)
{
  create_edge_ = new NodeViewEdge();
  create_edge_src_ = output_item;
  create_edge_src_output_ = Node::kDefaultOutput;
  create_edge_already_exists_ = false;

  create_edge_->SetCurved(scene_.GetEdgesAreCurved());
  create_edge_->SetFlowDirection(scene_.GetFlowDirection());

  scene_.addItem(create_edge_);
}

void NodeView::RepositionContexts()
{
  // Determine which contexts are root-level
  QVector<Node*> processing_filters = filter_nodes_;

  // Level counter as we iterate through the list a few times
  int level = 0;

  // Root-level positioning variables
  qreal last_offset = 0;
  int additional_spacing = 0;

  while (!processing_filters.isEmpty()) {
    QVector<Node*> contexts_on_this_level;

    for (int i=0; i<processing_filters.size(); i++) {
      Node *context = processing_filters.at(i);

      // Determine if this context is on an upper level or not
      bool this_level = true;
      for (int j=0; j<processing_filters.size(); j++) {
        Node *other_context = processing_filters.at(j);

        if (i != j && graph_->ContextContainsNode(context, other_context)) {
          this_level = false;
          break;
        }
      }

      if (this_level) {
        contexts_on_this_level.append(context);
      }
    }

    for (Node *context : qAsConst(contexts_on_this_level)) {
      if (level == 0) {
        const NodeGraph::PositionMap &map = graph_->GetNodesForContext(context);

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
        context_offsets_.insert(context, QPointF(0, last_offset));
      } else {
        // Create/update item
        NodeViewItem *item = UpdateNodeItem(context, true);

        // Get position generated by UpdateNodeItem
        QPointF context_pos = item->GetNodePosition();

        // Adjust by the context node's position in its own context (this will usually be 0,0)
        context_pos -= graph_->GetNodesForContext(context).value(context);

        // Insert this context's offset
        context_offsets_.insert(context, context_pos);
      }

      // Remove from list so we don't process again
      processing_filters.removeOne(context);
    }

    level++;
  }

  // Now that we've positioned all the contexts, position all other nodes relative to those contexts
  for (Node *context : qAsConst(filter_nodes_)) {
    const NodeGraph::PositionMap &map = graph_->GetNodesForContext(context);
    for (auto it=map.cbegin(); it!=map.cend(); it++) {
      UpdateNodeItem(it.key());
    }
  }
}

NodeViewItem *NodeView::UpdateNodeItem(Node *node, bool ignore_own_context)
{
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
  for (Node *context : qAsConst(filter_nodes_)) {
    if (context == node && ignore_own_context) {
      continue;
    }

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

  return item;
}

void NodeView::PasteNodesInternal(const QVector<Node *> &duplicate_nodes)
{
  // If no graph, do nothing
  if (!graph_) {
    return;
  }

  paste_command_ = new MultiUndoCommand();

  // If duplicating nodes, duplicate, otherwise paste
  QVector<Node*> new_nodes;
  NodeGraph::PositionMap map;
  if (duplicate_nodes.isEmpty()) {
    new_nodes = PasteNodesFromClipboard(graph_, paste_command_, &map);

    for (auto it=new_nodes.cbegin(); it!=new_nodes.cend(); it++) {
      for (Node *context : qAsConst(filter_nodes_)) {
        paste_command_->add_child(new NodeSetPositionCommand(*it, context, map.value(*it), false));
      }
    }
  } else {
    new_nodes = Node::CopyDependencyGraph(duplicate_nodes, paste_command_);

    for (int i=0; i<duplicate_nodes.size(); i++) {
      Node *src = duplicate_nodes.at(i);
      Node *copy = new_nodes.at(i);

      for (Node *context : qAsConst(filter_nodes_)) {
        QPointF p = scene_.item_map().value(src)->GetNodePosition();
        paste_command_->add_child(new NodeSetPositionCommand(copy, context, p, false));
      }
    }
  }

  // If no nodes were retrieved, do nothing
  if (new_nodes.isEmpty()) {
    delete paste_command_;
    paste_command_ = nullptr;
    return;
  }

  // Attach nodes to cursor
  paste_command_->add_child(new NodeViewAttachNodesToCursor(this, new_nodes));

  paste_command_->redo();
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

void NodeView::NodeViewItemPreventRemovingCommand::redo()
{
  NodeViewItem *item = view_->scene_.item_map().value(node_);

  if (item) {
    old_prevent_removing_ = item->GetPreventRemoving();
    item->SetPreventRemoving(new_prevent_removing_);
  }
}

void NodeView::NodeViewItemPreventRemovingCommand::undo()
{
  NodeViewItem *item = view_->scene_.item_map().value(node_);

  if (item) {
    item->SetPreventRemoving(old_prevent_removing_);
  }
}

}
