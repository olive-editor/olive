/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollBar>
#include <QToolTip>

#include "node/audio/volume/volume.h"
#include "node/distort/transform/transformdistortnode.h"
#include "node/factory.h"
#include "node/group/group.h"
#include "node/nodeundo.h"
#include "node/project/serializer/serializer.h"
#include "node/traverser.h"
#include "ui/icons/icons.h"
#include "widget/menu/menushared.h"
#include "widget/timebased/timebasedview.h"

#define super HandMovableView

namespace olive {

const double NodeView::kMinimumScale = 0.1;
const int NodeView::kMaximumContexts = 10;

NodeView::NodeView(QWidget *parent) :
  HandMovableView(parent),
  drop_edge_(nullptr),
  create_edge_(nullptr),
  create_edge_output_item_(nullptr),
  create_edge_input_item_(nullptr),
  overlay_view_(nullptr),
  scale_(1.0),
  dont_emit_selection_signals_(false)
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
  if (overlay_view_) {
    CloseOverlay();
  }

  // Remove contexts that are no longer in the list
  foreach (Node *n, contexts_) {
    if (!nodes.contains(n)) {
      RemoveContext(n);
    }
  }

  // Add contexts that are now in the list
  foreach (Node *n, nodes) {
    if (scene_.context_map().size() >= kMaximumContexts) {
      break;
    }

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
  NodeViewDeleteCommand* command = new NodeViewDeleteCommand();

  int count = 0;

  foreach (NodeViewContext *ctx, scene_.context_map()) {
    count += ctx->DeleteSelected(command);
  }

  Core::instance()->undo_stack()->push(command, tr("Deleted %1 Node(s)").arg(count));
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
  emit NodeSelectionChanged(selected_nodes_);
  emit NodeSelectionChangedWithContexts(QVector<Node::ContextPair>());
}

void NodeView::Select(const QVector<Node::ContextPair> &nodes, bool center_view_on_item)
{
  // Optimization: rather than respond to every single item being selected, ignore the signal and
  //               then handle them all at the end.
  DisconnectSelectionChangedSignal();

  QVector<Node*> deselections = selected_nodes_;
  QVector<Node*> new_selections;

  scene_.DeselectAll();

  foreach (const Node::ContextPair &p, nodes) {
    NodeViewContext *ctx = scene_.context_map().value(p.context);
    if (ctx) {
      NodeViewItem *item = ctx->GetItemFromMap(p.node);
      if (item) {
        item->setSelected(true);
      }
    }
  }

  // Center on something
  if (center_view_on_item && !nodes.isEmpty()) {
    QMetaObject::invokeMethod(this, "CenterOnNode", Qt::QueuedConnection, OLIVE_NS_ARG(Node*, nodes.first().node));
  }

  ConnectSelectionChangedSignal();

  // Don't signal when this function was likely triggered from another widget's signal anyway
  dont_emit_selection_signals_ = true;
  UpdateSelectionCache();
  dont_emit_selection_signals_ = false;
}

void NodeView::CopySelected(bool cut)
{
  if (selected_nodes_.isEmpty()) {
    return;
  }

  QString copy_str;
  QXmlStreamWriter writer(&copy_str);

  ProjectSerializer::SaveData sdata(ProjectSerializer::kOnlyNodes);
  sdata.SetOnlySerializeNodesAndResolveGroups(selected_nodes_);

  ProjectSerializer::SerializedProperties properties;

  for (Node *n : selected_nodes_) {
    NodeViewItem *item = GetAssumedItemForSelectedNode(n);

    if (item) {
      Node::Position pos = item->GetNodePositionData();

      properties[n][QStringLiteral("x")] = QString::number(pos.position.x());
      properties[n][QStringLiteral("y")] = QString::number(pos.position.y());
      properties[n][QStringLiteral("expanded")] = QString::number(pos.expanded);
    }
  }

  sdata.SetProperties(properties);

  ProjectSerializer::Save(&writer, sdata);

  Core::CopyStringToClipboard(copy_str);

  if (cut) {
    DeleteSelected();
  }
}

void NodeView::Paste()
{
  if (contexts_.isEmpty()) {
    return;
  }

  ProjectSerializer::Result res = ProjectSerializer::Paste(ProjectSerializer::kOnlyNodes);
  if (res.GetLoadData().nodes.isEmpty()) {
    return;
  }

  Node::PositionMap map;

  for (auto it=res.GetLoadData().properties.cbegin(); it!=res.GetLoadData().properties.cend(); it++) {
    Node::Position pos;

    const QMap<QString, QString> &node_props = it.value();
    pos.position.setX(node_props.value(QStringLiteral("x")).toDouble());
    pos.position.setY(node_props.value(QStringLiteral("y")).toDouble());
    pos.expanded = node_props.value(QStringLiteral("expanded")).toDouble();

    map.insert(it.key(), pos);
  }

  PostPaste(res.GetLoadData().nodes, map);
}

void NodeView::Duplicate()
{
  if (!selected_nodes_.isEmpty()) {
    QVector<Node*> selected = selected_nodes_;
    QVector<Node*> new_nodes;
    Node::PositionMap map;

    new_nodes.resize(selected.size());

    // Create copies of each selected node, checking for groups and adding children if necessary
    for (int i=0; i<selected.size(); i++) {
      new_nodes[i] = selected.at(i)->copy();

      if (NodeGroup *g = dynamic_cast<NodeGroup*>(selected.at(i))) {
        for (auto it=g->GetContextPositions().cbegin(); it!=g->GetContextPositions().cend(); it++) {
          if (!selected.contains(it.key())) {
            // This should automatically recurse if this is a group inside a group
            selected.append(it.key());
          }
        }
        new_nodes.resize(selected.size());
      }
    }

    // Get positions in contexts, add input passthroughs, and copy input values/keyframes
    for (int i=0; i<new_nodes.size(); i++) {
      Node *og = selected.at(i);
      Node *copy = new_nodes.at(i);

      Node::Position pos;
      if (GetAssumedPositionForSelectedNode(og, &pos)) {
        map.insert(copy, pos);
      }

      for (auto it=og->GetContextPositions().cbegin(); it!=og->GetContextPositions().cend(); it++) {
        Node *child_og = it.key();
        int child_index = selected.indexOf(child_og);

        if (child_index != -1) {
          Node *child_copy = new_nodes.at(child_index);

          copy->SetNodePositionInContext(child_copy, it.value());
        }
      }

      if (NodeGroup *src_group = dynamic_cast<NodeGroup*>(og)) {
        NodeGroup *dst_group = static_cast<NodeGroup*>(copy);

        for (auto it=src_group->GetInputPassthroughs().cbegin(); it!=src_group->GetInputPassthroughs().cend(); it++) {
          NodeInput input = it->second;
          input.set_node(new_nodes.at(selected.indexOf(input.node())));
          dst_group->AddInputPassthrough(input, it->first);
        }

        dst_group->SetOutputPassthrough(new_nodes.at(selected.indexOf(src_group->GetOutputPassthrough())));
      }

      Node::CopyInputs(selected.at(i), new_nodes.at(i), false);
    }

    // Copy connections
    Node::CopyDependencyGraph(selected, new_nodes, nullptr);

    // Set root level context positions and attach to
    PostPaste(new_nodes, map);
  }
}

void NodeView::SetColorLabel(int index)
{
  MultiUndoCommand *command = new MultiUndoCommand();

  for (Node* node : qAsConst(selected_nodes_)) {
    command->add_child(new NodeOverrideColorCommand(node, index));
  }

  Core::instance()->undo_stack()->push(command, tr("Set Color of %1 Node(s)").arg(selected_nodes_.size()));
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
    Core::instance()->undo_stack()->push(pos_command, tr("Moved %1 Node(s)").arg(selected_nodes_.size()));
    break;
  }
  case Qt::Key_Escape:
    if (!attached_items_.isEmpty()) {
      DetachItemsFromCursor();
      break;
    }

    emit EscPressed();

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
    // Sane defaults
    create_edge_already_exists_ = false;
    create_edge_from_output_ = true;
    create_edge_input_.Reset();

    if (event->modifiers() & Qt::ControlModifier) {
      NodeViewItem *mouse_item = dynamic_cast<NodeViewItem*>(item);

      if (mouse_item) {
        if (mouse_item->IsOutputItem()) {
          create_edge_output_item_ = mouse_item;
        } else {
          create_edge_input_item_ = mouse_item;
          create_edge_input_ = mouse_item->GetInput();
          create_edge_from_output_ = false;
        }

        // Highlight start item for better user experience
        mouse_item->SetHighlighted(true);
      }
    }

    if (!create_edge_output_item_ && !create_edge_input_item_) {
      // Determine if user clicked on a connector
      if (NodeViewItemConnector *connector = dynamic_cast<NodeViewItemConnector *>(item)) {
        NodeViewItem *attached = static_cast<NodeViewItem*>(connector->parentItem());

        if (connector->IsOutput()) {
          create_edge_output_item_ = attached;
        } else {
          create_edge_input_item_ = attached;

          if (!create_edge_input_item_->edges().isEmpty()) {
            // Drag existing edge instead
            create_edge_ = create_edge_input_item_->edges().first();
            create_edge_input_item_ = nullptr;
            create_edge_output_item_ = create_edge_->from_item();
            create_edge_already_exists_ = true;
          } else {
            create_edge_from_output_ = false;
            create_edge_input_ = create_edge_input_item_->GetInput();
          }
        }
      }
    }

    if ((create_edge_output_item_ || create_edge_input_item_) && !create_edge_already_exists_) {
      // Create a new edge from this output
      create_edge_ = new NodeViewEdge();
      create_edge_->SetCurved(scene_.GetEdgesAreCurved());

      // Add edge to scene
      scene_.addItem(create_edge_);

      // Position edge to mouse cursor
      PositionNewEdge(event->pos());
      return;
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

  if (attached_items_.isEmpty()) {
    // Default QGraphicsView functionality (selecting, dragging, etc.)
    super::mousePressEvent(event);
  }

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

  if (attached_items_.isEmpty()) {
    super::mouseMoveEvent(event);
  }

  // See if there are any items attached
  if (!attached_items_.isEmpty()) {
    ProcessMovingAttachedNodes(event->pos());
  }
}

void NodeView::mouseReleaseEvent(QMouseEvent *event)
{
  if (HandRelease(event)) return;

  if (create_edge_) {
    EndEdgeDrag();
  }

  MultiUndoCommand* command = new MultiUndoCommand();

  Node *select_context = nullptr;
  QVector<Node*> select_nodes;

  bool had_attached_items = !attached_items_.isEmpty();

  if (!attached_items_.isEmpty()) {
    select_context = GetContextAtMousePos(event->pos());

    if (select_context) {
      select_nodes = ProcessDroppingAttachedNodes(command, select_context, event->pos());
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

  Core::instance()->undo_stack()->push(command, tr("Moved %1 Node(s)").arg(dragging_items_.size()));

  dragging_items_.clear();

  if (!had_attached_items) {
    super::mouseReleaseEvent(event);
  }

  if (select_context) {
    DeselectAll();
    scene_.context_map().value(select_context)->Select(select_nodes);
  }
}

void NodeView::mouseDoubleClickEvent(QMouseEvent *event)
{
  super::mouseDoubleClickEvent(event);

  if (!(event->modifiers() & Qt::ControlModifier)) {
    NodeViewItem *item_at_cursor = dynamic_cast<NodeViewItem*>(itemAt(event->pos()));
    if (item_at_cursor) {
      item_at_cursor->ToggleExpanded();
    }
  }
}

void NodeView::dragEnterEvent(QDragEnterEvent *event)
{
  if (contexts_.empty()) {
    event->ignore();
    return;
  }

  QStringList mime_fmts = event->mimeData()->formats();

  if (mime_fmts.contains(Project::kItemMimeType)) {
    QByteArray model_data = event->mimeData()->data(Project::kItemMimeType);
    QDataStream stream(&model_data, QIODevice::ReadOnly);

    // Variables to deserialize into
    quintptr item_ptr;
    QVector<Track::Reference> enabled_streams;
    QVector<AttachedItem> new_attached;

    int y = 0;

    while (!stream.atEnd()) {
      stream >> enabled_streams >> item_ptr;

      // Get Item object
      Node* item = reinterpret_cast<Node*>(item_ptr);

      if (ViewerOutput* f = dynamic_cast<ViewerOutput*>(item)) {
        NodeViewItem *new_item;

        new_item = new NodeViewItem(f, nullptr);
        new_item->SetFlowDirection(scene_.GetFlowDirection());
        new_item->SetNodePosition(QPointF(0, y));
        y++;
        scene_.addItem(new_item);

        new_attached.append({new_item, f, new_item->pos()});
      }
    }

    if (new_attached.empty()) {
      event->ignore();
    } else {
      SetAttachedItems(new_attached);

      event->accept();
    }
  }
}

void NodeView::dragMoveEvent(QDragMoveEvent *event)
{
  if (attached_items_.empty()) {
    event->ignore();
  } else {
    ProcessMovingAttachedNodes(event->pos());

    if (GetContextAtMousePos(event->pos())) {
      event->accept();
    } else {
      event->ignore();
    }
  }
}

void NodeView::dropEvent(QDropEvent *event)
{
  if (Node *drop_ctx = GetContextAtMousePos(event->pos())) {
    MultiUndoCommand *command = new MultiUndoCommand();
    QVector<Node*> select_nodes = ProcessDroppingAttachedNodes(command, drop_ctx, event->pos());
    Core::instance()->undo_stack()->push(command, tr("Dropped %1 Node(s)").arg(select_nodes.size()));

    DeselectAll();
    scene_.context_map().value(drop_ctx)->Select(select_nodes);

    event->accept();
  } else {
    DetachItemsFromCursor(false);
    event->ignore();
  }
}

void NodeView::dragLeaveEvent(QDragLeaveEvent *event)
{
  if (attached_items_.empty()) {
    event->ignore();
  } else {
    DetachItemsFromCursor(false);

    event->accept();
  }
}

void NodeView::resizeEvent(QResizeEvent *event)
{
  super::resizeEvent(event);

  RepositionMiniMap();

  if (overlay_view_) {
    ResizeOverlay();
  }
}

void NodeView::UpdateSelectionCache()
{
  QVector<NodeViewItem*> current_selection = scene_.GetSelectedItems();

  QVector<Node*> selected;
  QVector<Node*> deselected;

  QVector<Node::ContextPair> sel_with_ctx(current_selection.size());

  // Determine which nodes are newly selected
  for (int j=0; j<current_selection.size(); j++) {
    NodeViewItem *i = current_selection.at(j);
    Node *n = i->GetNode();
    if (!selected_nodes_.contains(n)) {
      selected.append(n);
      selected_nodes_.append(n);
    }

    sel_with_ctx[j] = {n, i->GetContext()};
  }

  // Determine which nodes are newly deselected
  if (current_selection.isEmpty()) {
    // All nodes that were selected have been deselected, so we'll just set them all to `deselected`
    deselected = selected_nodes_;
    selected_nodes_.clear();
  } else {
    foreach (Node* n, selected_nodes_) {
      bool still_selected = false;

      foreach (NodeViewItem *i, current_selection) {
        if (i->GetNode() == n) {
          still_selected = true;
          break;
        }
      }

      if (!still_selected) {
        deselected.append(n);
        selected_nodes_.removeOne(n);
      }
    }
  }

  if (!deselected.isEmpty()) {
    emit NodesDeselected(deselected);
  }

  if (!selected.isEmpty()) {
    emit NodesSelected(selected);
  }

  if (!dont_emit_selection_signals_) {
    emit NodeSelectionChanged(selected_nodes_);
    emit NodeSelectionChangedWithContexts(sel_with_ctx);
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

    // Show in Viewer option for nodes based on Viewer
    if (ViewerOutput* viewer = dynamic_cast<ViewerOutput*>(selected.first()->GetNode())) {
      Q_UNUSED(viewer)
      m.addSeparator();
      QAction* open_in_viewer_action = m.addAction(tr("Open in Viewer"));
      connect(open_in_viewer_action, &QAction::triggered, this, &NodeView::OpenSelectedNodeInViewer);
    }

    m.addSeparator();

    // Properties
    QAction *properties_action = m.addAction(tr("P&roperties"));
    connect(properties_action, &QAction::triggered, this, &NodeView::ShowNodeProperties);

  } else {

    QAction* curved_action = m.addAction(tr("Smooth Edges"));
    curved_action->setCheckable(true);
    curved_action->setChecked(scene_.GetEdgesAreCurved());
    connect(curved_action, &QAction::triggered, &scene_, &NodeViewScene::SetEdgesAreCurved);

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

    QVector<AttachedItem> new_attached;

    new_attached.append({new_item, new_node, QPointF(0, 0)});

    if (NodeGroup *new_group = dynamic_cast<NodeGroup*>(new_node)) {
      for (auto it=new_group->GetContextPositions().cbegin(); it!=new_group->GetContextPositions().cend(); it++) {
        new_attached.append({nullptr, it.key(), QPointF(0, 0)});
      }
    }

    SetAttachedItems(new_attached);
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

void NodeView::CenterOnNode(Node *n)
{
  foreach (NodeViewContext *ctx, scene_.context_map()) {
    if (NodeViewItem* item = ctx->GetItemFromMap(n)) {
      centerOn(item);
      break;
    }
  }
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

void NodeView::DetachItemsFromCursor(bool delete_nodes_too)
{
  foreach (const AttachedItem &ai, attached_items_) {
    delete ai.item;

    if (delete_nodes_too) {
      qDebug() << "deleting" << ai.node;
      delete ai.node;
    }
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
    if (i.item) {
      i.item->setPos(item_pos + i.original_pos);
    }
  }
}

void NodeView::ProcessMovingAttachedNodes(const QPoint &pos)
{
  // Move those items to the cursor
  MoveAttachedNodesToCursor(pos);

  // See if the user clicked on an edge (only when dropping single nodes)
  if (attached_items_.size() == 1) {
    Node* attached_node = attached_items_.first().item->GetNode();

    QRect edge_detect_rect(pos, pos);

    int edge_detect_radius = fontMetrics().height();
    edge_detect_rect.adjust(-edge_detect_radius, -edge_detect_radius, edge_detect_radius, edge_detect_radius);

    QList<QGraphicsItem*> items = this->items(edge_detect_rect);

    NodeViewEdge* new_drop_edge = nullptr;

    // See if there is an edge here
    for (QGraphicsItem* item : qAsConst(items)) {
      new_drop_edge = dynamic_cast<NodeViewEdge*>(item);

      if (new_drop_edge) {
        drop_input_.Reset();

        NodeValue::Type drop_edge_data_type = new_drop_edge->input().GetDataType();

        // Determine best input to connect to our new node
        if (attached_node->GetEffectInput().IsValid()) {
          // If node specifies an effect input, use that immediately
          drop_input_ = attached_node->GetEffectInput();
        } else {
          // Otherwise, we may have to iterate to find a valid one
          for (const QString& input : attached_node->inputs()) {
            if (input == Node::kEnabledInput) {
              // Ignore enabled input
              continue;
            }

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
        }

        if (attached_node->InputsFrom(new_drop_edge->input().node(), true)) {
          drop_input_.Reset();
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

QVector<Node*> NodeView::ProcessDroppingAttachedNodes(MultiUndoCommand *command, Node *select_context, const QPoint &pos)
{
  QVector<Node*> select_nodes;

  // Make a copy
  QVector<AttachedItem> attached = attached_items_;

  for (int i=0; i<attached.size(); i++) {
    const AttachedItem &ai = attached.at(i);

    if (ai.node->InputsFrom(select_context, true)) {
      attached.removeAt(i);
    } else if (select_context->ContextContainsNode(ai.node)) {
      select_nodes.append(ai.node);
      attached.removeAt(i);
    }
  }

  {
    MultiUndoCommand *add_command = new MultiUndoCommand();

    foreach (const AttachedItem &ai, attached) {
      // Add node to the same graph that the context is in
      if (ai.node->parent() != select_context->parent()) {
        add_command->add_child(new NodeAddCommand(select_context->parent(), ai.node));
        if (ai.node->IsItem() && !ai.node->folder()) {
          add_command->add_child(new FolderAddChild(select_context->parent()->root(), ai.node));
        }
      }

      // Add node to the context
      if (ai.item) {
        select_nodes.append(ai.node);
        add_command->add_child(new NodeSetPositionCommand(ai.node, select_context, scene_.context_map().value(select_context)->MapScenePosToNodePosInContext(ai.item->pos())));
      }
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
    if (attached.size() == 1) {
      Node* dropping_node = nullptr;

      foreach (const AttachedItem &ai, attached) {
        if (ai.item && !ai.node->InputsFrom(select_context, true)) {
          dropping_node = ai.node;
          break;
        }
      }

      if (dropping_node && drop_edge_) {
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

  DetachItemsFromCursor(false);

  return select_nodes;
}

Node *NodeView::GetContextAtMousePos(const QPoint &p)
{
  QList<QGraphicsItem*> items_at_cursor = this->items(p);
  foreach (QGraphicsItem *i, items_at_cursor) {
    if (NodeViewContext *context_item = dynamic_cast<NodeViewContext*>(i)) {
      return context_item->GetContext();
    }
  }

  return nullptr;
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

void NodeView::changeEvent(QEvent *e)
{
  // Add translation code

  super::changeEvent(e);
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

void NodeView::ClearCreateEdgeInputIfNecessary()
{
  if (create_edge_from_output_ && create_edge_input_.IsValid()) {
    create_edge_input_.Reset();
  }
}

QPointF NodeView::GetEstimatedPositionForContext(NodeViewItem *item, Node *context) const
{
  return item->GetNodePosition() - context_offsets_.value(context);
}

NodeViewItem *NodeView::GetAssumedItemForSelectedNode(Node *node)
{
  // Try to find corresponding selected item
  foreach (NodeViewContext *ctx, scene_.context_map()) {
    NodeViewItem *item = ctx->GetItemFromMap(node);
    if (item && item->GetNode() == node && item->isSelected()) {
      // Good enough
      return item;
    }
  }

  return nullptr;
}

bool NodeView::GetAssumedPositionForSelectedNode(Node *node, Node::Position *pos)
{
  if (NodeViewItem *item = GetAssumedItemForSelectedNode(node)) {
    *pos = item->GetNodePositionData();
    return true;
  } else {
    return false;
  }
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
  if (item_at_cursor && item_at_cursor->GetNode() == source_item->GetNode()) {
    item_at_cursor = nullptr;
  }

  // Collapse any items that the cursor is no longer inside
  int i=create_edge_expanded_items_.size() - 1;
  for ( ; i>=0; i--) {
    NodeViewItem* nvi = create_edge_expanded_items_.at(i);
    QPointF local_pt = nvi->mapFromScene(scene_pt);

    if (nvi->scene() == &scene_ && (nvi->contains(local_pt) || (!nvi->IsOutputItem() && nvi->parentItem()->contains(nvi->parentItem()->mapFromScene(scene_pt)) && local_pt.y() > nvi->rect().bottom()))) {
      break;
    } else {
      // Collapsing an item will destroy its children, so if the cursor item happens to be a child
      // of the item we're about to collapse, set it to null
      if (item_at_cursor && item_at_cursor->parentItem() == nvi) {
        item_at_cursor = nullptr;
      }

      if (opposing_item && opposing_item->parentItem() == nvi) {
        opposing_item = nullptr;
        ClearCreateEdgeInputIfNecessary();
      }

      CollapseItem(nvi);
    }
  }
  create_edge_expanded_items_.resize(i + 1);

  // Expand item if possible
  if (item_at_cursor
      && item_at_cursor->CanBeExpanded()
      && !item_at_cursor->IsExpanded()
      && create_edge_from_output_) {
    ExpandItem(item_at_cursor);
    create_edge_expanded_items_.append(item_at_cursor);
  }

  // Filter out connecting to a node that connects to us or an item of the same type
  if (item_at_cursor
      && ((create_edge_from_output_ && source_item->GetNode()->InputsFrom(item_at_cursor->GetNode(), true))
          || (!create_edge_from_output_ && item_at_cursor->GetNode()->InputsFrom(source_item->GetNode(), true))
          || (create_edge_from_output_ == item_at_cursor->IsOutputItem()))) {
    item_at_cursor = nullptr;
  }

  // Filter out "output node" of the context, we assume users won't want to fetch the output of this
  if (item_at_cursor && !create_edge_from_output_ && item_at_cursor->IsLabelledAsOutputOfContext()) {
    item_at_cursor = nullptr;
  }

  // If the item has changed
  if (item_at_cursor != opposing_item) {
    // If we had a destination active, disconnect from it since the item has changed
    if (opposing_item) {
      opposing_item->SetHighlighted(false);
      opposing_item = nullptr;
    }

    // Clear cached input
    ClearCreateEdgeInputIfNecessary();

    // If this is an input and we're
    opposing_item = item_at_cursor;

    if (opposing_item) {
      opposing_item->SetHighlighted(true);
      if (!opposing_item->IsOutputItem()) {
        create_edge_input_ = opposing_item->GetInput();
      }
    }
  }

  QPointF output_point = create_edge_output_item_ ? create_edge_output_item_->GetOutputPoint() : scene_pt;
  QPointF input_point = create_edge_input_.IsValid() ? create_edge_input_item_->GetInputPoint() : scene_pt;

  create_edge_->SetPoints(output_point, input_point);
  create_edge_->SetConnected(create_edge_output_item_ && create_edge_input_.IsValid());
}

void NodeView::GroupNodes()
{
  // Get items
  QVector<NodeViewItem*> items = scene_.GetSelectedItems();
  if (items.isEmpty()) {
    return;
  }

  // Get node context
  Node *context = items.first()->GetContext();
  QPointF avg_pos = items.first()->GetNodePosition();
  for (int i=1; i<items.size(); i++) {
    if (items.at(i)->GetContext() != context) {
      QMessageBox::critical(this, tr("Failed to group nodes"), tr("Nodes can only be grouped if they're in the same context."));
      return;
    }

    avg_pos += items.at(i)->GetNodePosition();
  }
  avg_pos /= items.size();

  // Create group
  NodeGroup *group = new NodeGroup();

  // Add group to graph and context
  MultiUndoCommand *command = new MultiUndoCommand();

  // Add nodes to group
  Node *output_passthrough = nullptr;
  QVector<Node*> nodes_to_group = selected_nodes_;
  DeselectAll();
  foreach (Node *n, nodes_to_group) {
    command->add_child(new NodeRemovePositionFromContextCommand(n, context));
    command->add_child(new NodeSetPositionCommand(n, group, context->GetNodePositionDataInContext(n)));

    for (auto it=n->inputs().cbegin(); it!=n->inputs().cend(); it++) {
      NodeInput input(n, *it, -1);

      if (!input.IsConnected() || !nodes_to_group.contains(input.GetConnectedOutput())) {
        command->add_child(new NodeGroupAddInputPassthrough(group, input));
      }
    }

    if (!output_passthrough) {
      // Default to the first node we find that doesn't output to a node inside the group
      output_passthrough = nodes_to_group.first();
      foreach (Node *potential_in, nodes_to_group) {
        if (potential_in != n && !potential_in->InputsFrom(n, false)) {
          output_passthrough = n;
          break;
        }
      }
    }
  }

  // Set output passthrough
  command->add_child(new NodeGroupSetOutputPassthrough(group, output_passthrough));

  // Add group to graph
  command->add_child(new NodeAddCommand(context->parent(), group));
  command->add_child(new NodeSetPositionCommand(group, context, avg_pos));

  // Do command
  Core::instance()->LabelNodes({group}, command);

  Core::instance()->undo_stack()->push(command, tr("Grouped Nodes"));
}

void NodeView::UngroupNodes()
{
  NodeViewItem *group_item = nullptr;
  QVector<NodeViewItem*> items = scene_.GetSelectedItems();
  if (items.isEmpty()) {
    return;
  }

  NodeGroup *group = nullptr;
  foreach (NodeViewItem *i, items) {
    if ((group = dynamic_cast<NodeGroup*>(i->GetNode()))) {
      group_item = i;
      break;
    }
  }

  if (!group_item) {
    return;
  }

  MultiUndoCommand *command = new MultiUndoCommand();

  Node *context = group_item->GetContext();

  command->add_child(new NodeRemovePositionFromContextCommand(group, context));
  command->add_child(new NodeRemoveAndDisconnectCommand(group));

  for (auto it=group->GetContextPositions().cbegin(); it!=group->GetContextPositions().cend(); it++) {
    command->add_child(new NodeRemovePositionFromContextCommand(it.key(), group));
    command->add_child(new NodeSetPositionCommand(it.key(), context, group->GetNodePositionDataInContext(it.key())));
  }

  Core::instance()->undo_stack()->push(command, tr("Ungrouped Nodes"));
}

void NodeView::ShowNodeProperties()
{
  Node *first_node = selected_nodes_.first();

  if (NodeGroup *group = dynamic_cast<NodeGroup*>(first_node)) {
    if (!overlay_view_) {
      overlay_view_ = new NodeView(this);
      overlay_view_->show();

      QPushButton *overlay_close_btn = new QPushButton(overlay_view_);
      overlay_close_btn->setIcon(icon::Error);
      int offset = overlay_close_btn->sizeHint().width()/2;
      overlay_close_btn->move(offset, offset);
      overlay_close_btn->show();

      connect(overlay_view_, &NodeView::NodesSelected, this, &NodeView::NodesSelected);
      connect(overlay_view_, &NodeView::NodesDeselected, this, &NodeView::NodesDeselected);
      connect(overlay_view_, &NodeView::NodeGroupOpened, this, &NodeView::NodeGroupOpened);
      connect(overlay_view_, &NodeView::NodeGroupClosed, this, &NodeView::NodeGroupClosed);
      connect(overlay_view_, &NodeView::EscPressed, this, &NodeView::CloseOverlay);
      connect(overlay_close_btn, &QPushButton::clicked, this, &NodeView::CloseOverlay);

      const QColor &bgcol = overlay_view_->palette().base().color();
      overlay_view_->setStyleSheet(QStringLiteral("QGraphicsView { background: rgba(%1, %2, %3, 0.8); }").arg(QString::number(bgcol.red()), QString::number(bgcol.green()), QString::number(bgcol.blue())));

      overlay_close_btn->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }
    overlay_view_->SetContexts({group});
    ResizeOverlay();
    QMetaObject::invokeMethod(overlay_view_, &NodeView::CenterOnItemsBoundingRect, Qt::QueuedConnection);
    overlay_view_->setFocus();

    emit NodesDeselected(selected_nodes_);
    emit NodeSelectionChanged(QVector<Node*>());
    emit NodeSelectionChangedWithContexts(QVector<Node::ContextPair>());
    overlay_view_->SelectAll();

    emit NodeGroupOpened(group);
  } else {
    LabelSelectedNodes();
  }
}

void NodeView::LabelSelectedNodes()
{
  Core::instance()->LabelNodes(selected_nodes_);
}

void NodeView::ItemAboutToBeDeleted(NodeViewItem *item)
{
  dragging_items_.remove(item);

  if (create_edge_) {
    // Item should be removed from scene, but not yet deleted, allowing a safe PositionNewEdge call
    // to disconnect
    PositionNewEdge(mapFromGlobal(QCursor::pos()));

    QGraphicsItem *test = item;
    do {
      if (test == item) {
        break;
      }

      test = test->parentItem();
    } while (test);

    if (test == item) {
      // Cancel edge function
      EndEdgeDrag(true);
    }
  }
}

void NodeView::CloseOverlay()
{
  if (overlay_view_->overlay_view_) {
    overlay_view_->CloseOverlay();
  }

  overlay_view_->deleteLater();
  overlay_view_ = nullptr;
  emit NodeGroupClosed();
}

void NodeView::AddContext(Node *n)
{
  NodeViewContext *ctx = scene_.AddContext(n);

  connect(ctx, &NodeViewContext::ItemAboutToBeDeleted, this, &NodeView::ItemAboutToBeDeleted);

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

void NodeView::ExpandItem(NodeViewItem *item)
{
  item->SetExpanded(true);
  item->setZValue(100);
}

void NodeView::CollapseItem(NodeViewItem *item)
{
  item->SetExpanded(false);
  item->setZValue(0);
}

void NodeView::EndEdgeDrag(bool cancel)
{
  // Check if the edge was reconnected to the same place as before
  MultiUndoCommand* command = new MultiUndoCommand();

  bool reconnected_to_itself = false;

  if (create_edge_already_exists_) {
    if (!cancel) {
      if (create_edge_output_item_ == create_edge_->from_item() && create_edge_->input() == create_edge_input_) {
        reconnected_to_itself = true;
      } else {
        // We are moving (or removing) an existing edge
        command->add_child(new NodeEdgeRemoveCommand(create_edge_->output(), create_edge_->input()));
      }
    }
  } else {
    // We're creating a new edge, which means this UI object is only temporary
    delete create_edge_;
  }

  create_edge_ = nullptr;

  // Clear highlight if we set one
  if (create_edge_output_item_) {
    create_edge_output_item_->SetHighlighted(false);
  }
  if (create_edge_input_item_) {
    create_edge_input_item_->SetHighlighted(false);
  }

  QString command_name;

  NodeInput &creating_input = create_edge_input_;
  if (create_edge_output_item_ && create_edge_input_item_ && !cancel) {
    if (creating_input.IsValid()) {
      // Make connection
      if (!reconnected_to_itself) {
        Node *creating_output = create_edge_output_item_->GetNode();

        while (NodeGroup *output_group = dynamic_cast<NodeGroup*>(creating_output)) {
          creating_output = output_group->GetOutputPassthrough();
        }

        while (NodeGroup *input_group = dynamic_cast<NodeGroup*>(creating_input.node())) {
          creating_input = input_group->GetInputFromID(creating_input.input());
        }

        if (creating_input.IsConnected()) {
          Node::OutputConnection existing_edge_to_remove = {creating_input.GetConnectedOutput(), creating_input};

          Node *already_connected_output = creating_input.GetConnectedOutput();
          NodeViewContext *ctx = GetContextItemFromNodeItem(create_edge_input_item_);
          if (ctx && !ctx->GetItemFromMap(already_connected_output)) {
            if (QMessageBox::warning(this, QString(), tr("Input \"%1\" is currently connected to node \"%2\", which is not visible in this context. "
                                                         "By connecting this, that connection will be removed. Do you wish to continue?").arg(creating_input.name(), already_connected_output->GetLabelAndName()),
                                     QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
              cancel = true;
            }
          }

          if (!cancel) {
            command->add_child(new NodeEdgeRemoveCommand(existing_edge_to_remove.first, existing_edge_to_remove.second));
          }
        }

        if (!cancel) {
          command->add_child(new NodeEdgeAddCommand(creating_output, creating_input));

          command_name = Node::GetConnectCommandString(creating_output, creating_input);

          // If the output is not in the input's context, add it now. We check the item rather than
          // the node itself, because sometimes a node may not be in the context but another node
          // representing it will be (e.g. groups)
          if (!scene_.context_map().value(create_edge_input_item_->GetContext())->GetItemFromMap(creating_output)) {
            command->add_child(new NodeSetPositionCommand(creating_output, create_edge_input_item_->GetContext(), scene_.context_map().value(create_edge_input_item_->GetContext())->MapScenePosToNodePosInContext(create_edge_output_item_->scenePos())));
          }
        }
      }

    }
  }

  creating_input.Reset();
  create_edge_output_item_ = nullptr;
  create_edge_input_item_ = nullptr;

  // Collapse any items we expanded
  for (auto it=create_edge_expanded_items_.crbegin(); it!=create_edge_expanded_items_.crend(); it++) {
    CollapseItem(*it);
  }
  create_edge_expanded_items_.clear();

  Core::instance()->undo_stack()->push(command, command_name);
}

void NodeView::PostPaste(const QVector<Node *> &new_nodes, const Node::PositionMap &map)
{
  QVector<AttachedItem> new_attached;

  NodeViewItem *first_item = nullptr;

  for (int i=0; i<new_nodes.size(); i++) {
    Node *node = new_nodes.at(i);

    // Determine if item had a position, if not don't create an item for it
    NodeViewItem *new_item;

    if (map.contains(node)) {
      new_item = new NodeViewItem(node, nullptr);
      new_item->SetFlowDirection(scene_.GetFlowDirection());
      new_item->SetNodePosition(map.value(node));
      scene_.addItem(new_item);

      if (!first_item) {
        first_item = new_item;
      }
    } else {
      new_item = nullptr;
    }

    new_attached.append({new_item, node, QPointF(0, 0)});
  }

  // Correct positions
  if (first_item) {
    for (int i=0; i<new_attached.size(); i++) {
      AttachedItem &ai = new_attached[i];

      if (ai.item) {
        ai.original_pos = ai.item->pos() - first_item->pos();
      }
    }
  }

  SetAttachedItems(new_attached);
}

void NodeView::ResizeOverlay()
{
  overlay_view_->resize(this->size());
}

NodeViewContext *NodeView::GetContextItemFromNodeItem(NodeViewItem *item)
{
  QGraphicsItem *i = item;
  while ((i = i->parentItem())) {
    if (NodeViewContext *nvc = dynamic_cast<NodeViewContext*>(i)) {
      return nvc;
    }
  }
  return nullptr;
}

void NodeView::SetAttachedItems(const QVector<AttachedItem> &items)
{
  // Detach anything currently attached
  DetachItemsFromCursor();

  attached_items_ = items;

  // Move to cursor
  MoveAttachedNodesToCursor(mapFromGlobal(QCursor::pos()));
}

}
