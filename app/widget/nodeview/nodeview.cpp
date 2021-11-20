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

#include <QInputDialog>
#include <QMouseEvent>
#include <QScrollBar>
#include <QToolTip>

#include "core.h"
#include "nodeviewundo.h"
#include "node/audio/volume/volume.h"
#include "node/distort/transform/transformdistortnode.h"
#include "node/factory.h"
#include "node/group.h"
#include "node/traverser.h"
#include "widget/menu/menushared.h"
#include "widget/timebased/timebasedview.h"

#define super HandMovableView

namespace olive {

const double NodeView::kMinimumScale = 0.1;

NodeView::NodeView(QWidget *parent) :
  HandMovableView(parent),
  drop_edge_(nullptr),
  create_edge_(nullptr),
  create_edge_output_item_(nullptr),
  create_edge_input_item_(nullptr),
  create_edge_dst_temp_expanded_(false),
  paste_command_(nullptr),
  scale_(1.0),
  first_show_(true)
{
  setScene(&scene_);
  SetDefaultDragMode(RubberBandDrag);
  setContextMenuPolicy(Qt::CustomContextMenu);
  setMouseTracking(true);
  setRenderHint(QPainter::Antialiasing);
  setViewportUpdateMode(FullViewportUpdate);

  connect(this, &NodeView::customContextMenuRequested, this, &NodeView::ShowContextMenu);

  ConnectSelectionChangedSignal();

  SetFlowDirection(NodeViewCommon::kLeftToRight);

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

void NodeView::SetContexts(const QVector<Node*> &nodes)
{
  // Remove contexts that are no longer in the list
  foreach (Node *n, contexts_) {
    if (!nodes.contains(n)) {
      RemoveContext(n);
    }
  }

  // Add contexts that are now in the list
  foreach (Node *n, nodes) {
    if (!contexts_.contains(n)) {
      AddContext(n);
    }
  }

  contexts_ = nodes;

  CenterOnItemsBoundingRect();
}

void NodeView::CloseContextsBelongingToProject(Project *project)
{
  QVector<Node*> new_contexts = contexts_;

  for (auto it = new_contexts.begin(); it != new_contexts.end(); ) {
    if ((*it)->project() == project) {
      it = new_contexts.erase(it);
    } else {
      it++;
    }
  }

  SetContexts(new_contexts);
}

void NodeView::ClearGraph()
{
  SetContexts(QVector<Node*>());
}

void NodeView::DeleteSelected()
{
  scene_.DeleteSelected();
}

void NodeView::SelectAll()
{
  // Optimization: rather than respond to every single item being selected, ignore the signal and
  //               then handle them all at the end.
  DisconnectSelectionChangedSignal();

  scene_.SelectAll();

  ConnectSelectionChangedSignal();

  UpdateSelectionCache();
}

void NodeView::DeselectAll()
{
  if (selected_nodes_.isEmpty()) {
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

void NodeView::Select(const QVector<Node *> &nodes, bool center_view_on_item)
{
  // Optimization: rather than respond to every single item being selected, ignore the signal and
  //               then handle them all at the end.
  DisconnectSelectionChangedSignal();

  QVector<Node*> deselections = selected_nodes_;
  QVector<Node*> new_selections;

  scene_.DeselectAll();

  foreach (NodeViewContext *context, scene_.context_map()) {
    context->Select(nodes);
  }

  /*
  // Center on something
  Node *first_item = nodes.isEmpty() ? nullptr : nodes.first();
  if (center_view_on_item && first_item) {
    centerOn(first_item);
  }
  */

  ConnectSelectionChangedSignal();

  UpdateSelectionCache();
}

void NodeView::CopySelected(bool cut)
{
  if (selected_nodes_.isEmpty()) {
    return;
  }

  CopyNodesToClipboard(selected_nodes_);

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
  PasteNodesInternal(selected_nodes_);
}

void NodeView::SetColorLabel(int index)
{
  MultiUndoCommand *command = new MultiUndoCommand();

  for (Node* node : qAsConst(selected_nodes_)) {
    command->add_child(new NodeOverrideColorCommand(node, index));
  }

  Core::instance()->undo_stack()->push(command);
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
    MultiUndoCommand *pos_command = new MultiUndoCommand();
    for (Node *n : qAsConst(selected_nodes_)) {
      for (Node *context : qAsConst(contexts_)) {
        if (context->ContextContainsNode(n)) {
          Node::Position old_pos = context->GetNodePositionInContext(n);

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
          pos_command->add_child(new NodeSetPositionCommand(n, context, old_pos + node_movement));
        }
      }
    }
    Core::instance()->undo_stack()->pushIfHasChildren(pos_command);
    break;
  }
  case Qt::Key_Escape:
    if (!attached_items_.isEmpty()) {
      DetachItemsFromCursor();

      // We undo the last action which SHOULD be adding the node
      if (paste_command_) {
        paste_command_->undo_now();
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
  // Handle mouse press event
  if (HandPress(event)) return;

  // Get the item that the user clicked on, if any
  QGraphicsItem* item = itemAt(event->pos());

  if (event->button() == Qt::LeftButton) {
    // Determine if user clicked on a connector
    NodeViewItemConnector *connector = dynamic_cast<NodeViewItemConnector *>(item);

    // If the user clicked on a connector OR the user is holding Ctrl
    if (connector || (event->modifiers() & Qt::ControlModifier)) {
      // Get the relevant item, either the one attached to the connector or the item if Ctrl+Clicked
      NodeViewItem *attached_item = connector ? static_cast<NodeViewItem*>(connector->parentItem()) : dynamic_cast<NodeViewItem*>(item);

      if (attached_item) {
        if (connector && !connector->IsOutput() && (create_edge_ = attached_item->GetEdgeFromInputConnector(connector))) {
          // Since inputs can only have one edge connected, we grab the existing edge, if one exists
          create_edge_output_item_ = create_edge_->from_item();
          create_edge_already_exists_ = true;
          create_edge_from_output_ = true;
        } else {
          // Create a new edge from this output
          create_edge_ = new NodeViewEdge();
          create_edge_->SetCurved(scene_.GetEdgesAreCurved());
          create_edge_->SetFlowDirection(scene_.GetFlowDirection());

          // Set source and declare that we created this edge
          if ((create_edge_from_output_ = (!connector || connector->IsOutput()))) {
            // Edge is being created from output
            create_edge_output_item_ = attached_item;
          } else {
            // Edge is being created from input
            create_edge_input_item_ = attached_item;
            create_edge_input_ = attached_item->GetInputFromInputConnector(connector);
          }
          create_edge_already_exists_ = false;

          // Add edge to scene
          scene_.addItem(create_edge_);

          // Position edge to mouse cursor
          PositionNewEdge(event->pos());
        }
        return;
      }
    }
  }

  // Handle selections with the right mouse button
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

  // Default QGraphicsView functionality (selecting, dragging, etc.)
  super::mousePressEvent(event);

  // For any selected item, store its position in case the user is dragging it somewhere else
  auto selected_items = scene_.GetSelectedItems();
  foreach (NodeViewItem *i, selected_items) {
    // Ignore items attached to the cursor
    if (!IsItemAttachedToCursor(i)) {
      dragging_items_.insert(i, i->GetNodePosition());
    }
  }
}

void NodeView::mouseMoveEvent(QMouseEvent *event)
{
  if (HandMove(event)) return;

  if (create_edge_) {
    PositionNewEdge(event->pos());
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

          // Run the Node and determine what type is being used
          NodeTraverser traverser;
          NodeValue drop_edge_value = traverser.GenerateRow(new_drop_edge->output(), TimeRange(0, 0))[new_drop_edge->input().input()];
          drop_edge_data_type = drop_edge_value.type();

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
    // Check if the edge was reconnected to the same place as before
    MultiUndoCommand* command = new MultiUndoCommand();

    bool reconnected_to_itself = false;

    if (create_edge_already_exists_) {
      if (create_edge_output_item_ == create_edge_->from_item() && create_edge_->input() == create_edge_input_) {
        reconnected_to_itself = true;
      } else {
        // We are moving (or removing) an existing edge
        command->add_child(new NodeEdgeRemoveCommand(create_edge_->output(), create_edge_->input()));
      }
    } else {
      // We're creating a new edge, which means this UI object is only temporary
      delete create_edge_;
    }

    create_edge_ = nullptr;

    if (create_edge_output_item_ && create_edge_input_item_) {
      // Clear highlight if we set one
      create_edge_input_item_->SetHighlightedIndex(-1);

      // Collapse if we expanded it
      if (create_edge_dst_temp_expanded_) {
        create_edge_input_item_->SetExpanded(false);
        create_edge_input_item_->setZValue(0);
      }

      NodeInput &creating_input = create_edge_input_;
      if (creating_input.IsValid()) {
        // Make connection
        if (!reconnected_to_itself) {
          Node *creating_output = create_edge_output_item_->GetNode();

          if (creating_input.IsConnected()) {
            Node::OutputConnection existing_edge_to_remove = {creating_input.GetConnectedOutput(), creating_input};
            command->add_child(new NodeEdgeRemoveCommand(existing_edge_to_remove.first, existing_edge_to_remove.second));
          }

          command->add_child(new NodeEdgeAddCommand(creating_output, creating_input));

          // If the output is not in the input's context, add it now
          if (!create_edge_input_item_->GetContext()->ContextContainsNode(creating_output)) {
            command->add_child(new NodeSetPositionCommand(creating_output, create_edge_input_item_->GetContext(), scene_.context_map().value(create_edge_input_item_->GetContext())->MapScenePosToNodePosInContext(create_edge_output_item_->scenePos())));
          }
        }

        creating_input.Reset();
      }

      create_edge_output_item_ = nullptr;
      create_edge_input_item_ = nullptr;
    }

    Core::instance()->undo_stack()->pushIfHasChildren(command);
  }

  MultiUndoCommand* command = new MultiUndoCommand();

  if (!attached_items_.isEmpty()) {
    Node *context = nullptr;

    QList<QGraphicsItem*> items_at_cursor = this->items(event->pos());
    foreach (QGraphicsItem *i, items_at_cursor) {
      if (NodeViewContext *context_item = dynamic_cast<NodeViewContext*>(i)) {
        context = context_item->GetContext();
        break;
      }
    }

    if (context) {
      if (paste_command_) {
        // We've already "done" this command, but MultiUndoCommand prevents "redoing" twice, so we
        // add it to this command (which may have extra commands added too) so that it all gets undone
        // in the same action
        command->add_child(paste_command_);
        paste_command_ = nullptr;
      }

      {
        MultiUndoCommand *add_command = new MultiUndoCommand();

        foreach (const AttachedItem &ai, attached_items_) {
          // Add node to the same graph that the context is in
          add_command->add_child(new NodeAddCommand(context->parent(), ai.item->GetNode()));

          // Add node to the context
          add_command->add_child(new NodeSetPositionCommand(ai.item->GetNode(), context, scene_.context_map().value(context)->MapScenePosToNodePosInContext(ai.item->pos())));
        }

        if (add_command->child_count()) {
          add_command->redo_now();
          command->add_child(add_command);
        } else {
          delete add_command;
        }
      }

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
          drop_edge_command->redo_now();
          command->add_child(drop_edge_command);
        } else {
          delete drop_edge_command;
        }
      }

      DetachItemsFromCursor();
    } else {
      QToolTip::showText(QCursor::pos(), tr("Nodes must be placed inside a context."));
    }
  }

  for (auto it=dragging_items_.cbegin(); it!=dragging_items_.cend(); it++) {
    NodeViewItem *i = it.key();
    QPointF current_pos = i->GetNodePosition();
    if (it.value() != current_pos) {
      command->add_child(new NodeSetPositionCommand(i->GetNode(), i->GetContext(), current_pos));
    }
  }
  dragging_items_.clear();

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
  QVector<NodeViewItem*> current_selection = scene_.GetSelectedItems();

  QVector<Node*> selected;
  QVector<Node*> deselected;

  // Determine which nodes are newly selected
  foreach (NodeViewItem* i, current_selection) {
    Node *n = i->GetNode();
    if (!selected_nodes_.contains(n)) {
      selected.append(n);
      selected_nodes_.append(n);
    }
  }

  // Determine which nodes are newly deselected
  if (current_selection.isEmpty()) {
    // All nodes that were selected have been deselected, so we'll just set them all to `deselected`
    deselected = selected_nodes_;
  } else {
    foreach (Node* n, selected_nodes_) {
      bool still_selected = false;

      foreach (NodeViewItem *i, current_selection) {
        if (i->GetNode() == n) {
          still_selected = true;
          break;
        }
      }

      if (still_selected) {
        deselected.append(n);
        selected_nodes_.removeOne(n);
      }
    }
  }

  if (!selected.isEmpty()) {
    emit NodesSelected(selected);
  }

  if (!deselected.isEmpty()) {
    emit NodesDeselected(deselected);
  }
}

void NodeView::ShowContextMenu(const QPoint &pos)
{
  if (contexts_.isEmpty()) {
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
      Core::instance()->LabelNodes(selected_nodes_);
    });

    // Grouping
    if (selected.size() == 1 && dynamic_cast<NodeGroup*>(selected.first()->GetNode())) {
      QAction *ungroup_action = m.addAction(tr("Ungroup"));
      connect(ungroup_action, &QAction::triggered, this, &NodeView::UngroupNodes);
    } else {
      QAction *group_action = m.addAction(tr("Group"));
      connect(group_action, &QAction::triggered, this, &NodeView::GroupNodes);
    }

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
  Node* new_node = NodeFactory::CreateFromMenuAction(action);

  if (new_node) {
    NodeViewItem *new_item = new NodeViewItem(new_node, nullptr);
    new_item->SetFlowDirection(scene_.GetFlowDirection());
    scene_.addItem(new_item);
    AttachItemsToCursor({new_item});
  }
}

void NodeView::ContextMenuSetDirection(QAction *action)
{
  SetFlowDirection(static_cast<NodeViewCommon::FlowDirection>(action->data().toInt()));
}

void NodeView::OpenSelectedNodeInViewer()
{
  // Find first viewer in list of selected nodes and open it
  foreach (Node *n, selected_nodes_) {
    if (ViewerOutput* viewer = dynamic_cast<ViewerOutput*>(n)) {
      Core::instance()->OpenNodeInViewer(viewer);
      break;
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

void NodeView::NodeRemovedFromGraph()
{
  Node *context = static_cast<Node*>(sender());

  RemoveContext(context);

  contexts_.removeOne(context);
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
  foreach (const AttachedItem &ai, attached_items_) {
    delete ai.item;
  }

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
  return super::eventFilter(object, event);
}

void NodeView::CopyNodesToClipboardInternal(QXmlStreamWriter *writer, const QVector<Node *> &nodes, void *userdata)
{
  qDebug() << "STUB!";
  /*writer->writeStartElement(QStringLiteral("pos"));

  for (Node *n : nodes) {
    NodeViewItem *item = scene_.item_map().value(n);
    QPointF pos = item->GetNodePosition();

    writer->writeStartElement(QStringLiteral("node"));
    writer->writeAttribute(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(n)));
    writer->writeTextElement(QStringLiteral("x"), QString::number(pos.x()));
    writer->writeTextElement(QStringLiteral("y"), QString::number(pos.y()));
    writer->writeEndElement(); // node
  }

  writer->writeEndElement(); // pos*/
}

void NodeView::PasteNodesFromClipboardInternal(QXmlStreamReader *reader, XMLNodeData &xml_node_data, void *userdata)
{
  qDebug() << "STUB!";
  /*NodeGraph::PositionMap *map = static_cast<NodeGraph::PositionMap *>(userdata);

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
  }*/
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

void NodeView::PositionNewEdge(const QPoint &pos)
{
  // Determine scene coordinate
  QPointF scene_pt = mapToScene(pos);

  // Find if the cursor is currently inside an item
  NodeViewItem* item_at_cursor = dynamic_cast<NodeViewItem*>(itemAt(pos));

  NodeViewItem *source_item = create_edge_from_output_ ? create_edge_output_item_ : create_edge_input_item_;
  NodeViewItem *&opposing_item = create_edge_from_output_ ? create_edge_input_item_ : create_edge_output_item_;

  // Filter out connecting to self
  if (item_at_cursor == source_item) {
    item_at_cursor = nullptr;
  }

  // Filter out connecting to a node that connects to us
  if (item_at_cursor
      && ((create_edge_from_output_ && item_at_cursor->GetNode()->OutputsTo(source_item->GetNode(), true))
          || (!create_edge_from_output_ && item_at_cursor->GetNode()->InputsFrom(source_item->GetNode(), true)))) {
    item_at_cursor = nullptr;
  }

  // If the item has changed
  if (item_at_cursor != opposing_item) {
    // If we had a destination active, disconnect from it since the item has changed
    if (opposing_item) {
      opposing_item->SetHighlightedIndex(-1);

      if (create_edge_dst_temp_expanded_) {
        // We expanded this item, so we can un-expand it
        opposing_item->SetExpanded(false);
        opposing_item->setZValue(0);
      }
    }

    // Set destination
    opposing_item = item_at_cursor;

    // If our destination is an item, ensure it's expanded
    if (opposing_item) {
      if (create_edge_from_output_ && (create_edge_dst_temp_expanded_ = (!create_edge_input_item_->IsExpanded()))) {
        create_edge_input_item_->SetExpanded(true, true);
        create_edge_input_item_->setZValue(100); // Ensure item is in front
      }
    }
  }

  // If we have a destination, highlight the appropriate input
  if (create_edge_from_output_) {
    int highlight_index = -1;
    if (create_edge_input_item_) {
      highlight_index = create_edge_input_item_->GetIndexAt(scene_pt);
      create_edge_input_item_->SetHighlightedIndex(highlight_index);
    }

    if (highlight_index >= 0) {
      create_edge_input_ = create_edge_input_item_->GetInputAtIndex(highlight_index);
    } else {
      create_edge_input_.Reset();
    }
  }

  QPointF output_point = create_edge_output_item_ ? create_edge_output_item_->GetOutputPoint() : scene_pt;
  QPointF input_point = create_edge_input_.IsValid() ? create_edge_input_item_->GetInputPoint(create_edge_input_.input(), create_edge_input_.element()) : scene_pt;

  create_edge_->SetPoints(output_point, input_point, create_edge_input_item_ && create_edge_input_item_->IsExpanded());
  create_edge_->SetConnected(create_edge_output_item_ && create_edge_input_.IsValid());
}

void NodeView::GroupNodes()
{
  /*NodeGroup *group = new NodeGroup();
  selected_nodes_*/
}

void NodeView::UngroupNodes()
{
  //static_cast<NodeGroup*>(selected_nodes_.first());
}

void NodeView::PasteNodesInternal(const QVector<Node *> &duplicate_nodes)
{
  /*
  // If no graph, do nothing
  if (contexts_.isEmpty()) {
    return;
  }

  paste_command_ = new MultiUndoCommand();

  // If duplicating nodes, duplicate, otherwise paste
  QVector<Node*> new_nodes;
  NodeGraph::PositionMap map;
  if (duplicate_nodes.isEmpty()) {
    new_nodes = PasteNodesFromClipboard(graph_, paste_command_, &map);

    for (auto it=new_nodes.cbegin(); it!=new_nodes.cend(); it++) {
      for (Node *context : qAsConst(contexts_)) {
        paste_command_->add_child(new NodeSetPositionCommand(*it, context, map.value(*it), false));
      }
    }
  } else {
    new_nodes = Node::CopyDependencyGraph(duplicate_nodes, paste_command_);

    for (int i=0; i<duplicate_nodes.size(); i++) {
      Node *src = duplicate_nodes.at(i);
      Node *copy = new_nodes.at(i);

      for (Node *context : qAsConst(contexts_)) {
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

  paste_command_->redo_now();
  */
}

void NodeView::AddContext(Node *n)
{
  scene_.AddContext(n);
  connect(n, &Node::RemovedFromGraph, this, &NodeView::NodeRemovedFromGraph);
}

void NodeView::RemoveContext(Node *n)
{
  scene_.RemoveContext(n);
  disconnect(n, &Node::RemovedFromGraph, this, &NodeView::NodeRemovedFromGraph);
}

bool NodeView::IsItemAttachedToCursor(NodeViewItem *item) const
{
  foreach (const AttachedItem &ai, attached_items_) {
    if (ai.item == item) {
      return true;
    }
  }

  return false;
}

}
