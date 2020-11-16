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

#include <QInputDialog>
#include <QMouseEvent>

#include "core.h"
#include "nodeviewundo.h"
#include "node/factory.h"
#include "widget/menu/menushared.h"

#define super HandMovableView

OLIVE_NAMESPACE_ENTER

NodeView::NodeView(QWidget *parent) :
  HandMovableView(parent),
  graph_(nullptr),
  drop_edge_(nullptr),
  filter_mode_(kFilterShowSelectedBlocks),
  scale_(1.0)
{
  setScene(&scene_);
  SetDefaultDragMode(RubberBandDrag);
  setContextMenuPolicy(Qt::CustomContextMenu);
  setMouseTracking(true);
  setRenderHint(QPainter::Antialiasing);
  setViewportUpdateMode(FullViewportUpdate);

  connect(&scene_, &QGraphicsScene::changed, this, &NodeView::ItemsChanged);
  connect(this, &NodeView::customContextMenuRequested, this, &NodeView::ShowContextMenu);

  ConnectSelectionChangedSignal();

  SetFlowDirection(NodeViewCommon::kTopToBottom);

  // Set massive scene rect and hide the scrollbars to create an "infinite space" effect
  scene_.setSceneRect(-1000000, -1000000, 2000000, 2000000);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  select_blocks_internal_timer_.setSingleShot(true);
  select_blocks_internal_timer_.setInterval(100);
  connect(&select_blocks_internal_timer_, &QTimer::timeout, this, &NodeView::SelectBlocksInternal);
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
    disconnect(graph_, &NodeGraph::NodeRemoved, this, &NodeView::GraphNodeRemoved);
    disconnect(graph_, &NodeGraph::EdgeAdded, this, &NodeView::GraphEdgeAdded);
    disconnect(graph_, &NodeGraph::EdgeRemoved, this, &NodeView::GraphEdgeRemoved);
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
    connect(graph_, &NodeGraph::NodeRemoved, this, &NodeView::GraphNodeRemoved);
    connect(graph_, &NodeGraph::EdgeAdded, this, &NodeView::GraphEdgeAdded);
    connect(graph_, &NodeGraph::EdgeRemoved, this, &NodeView::GraphEdgeRemoved);

    foreach (Node* n, graph_->nodes()) {
      scene_.AddNode(n);
    }

    ValidateFilter();
  }
}

void NodeView::DeleteSelected()
{
  if (!graph_) {
    return;
  }

  QUndoCommand* command = new QUndoCommand();

  {
    QVector<NodeEdge*> selected_edges = scene_.GetSelectedEdges();

    foreach (NodeEdge* edge, selected_edges) {
      new NodeEdgeRemoveCommand(edge->output(), edge->input(), command);
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
      new NodeRemoveCommand(graph_, selected_nodes, command);
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void NodeView::SelectAll()
{
  // Optimization: rather than respond to every single item being selected, ignore the signal and
  //               then handle them all at the end.
  DisconnectSelectionChangedSignal();

  scene_.SelectAll();

  ConnectSelectionChangedSignal();
  SceneSelectionChangedSlot();
}

void NodeView::DeselectAll()
{
  // Optimization: rather than respond to every single item being selected, ignore the signal and
  //               then handle them all at the end.
  DisconnectSelectionChangedSignal();

  scene_.DeselectAll();

  ConnectSelectionChangedSignal();
  SceneSelectionChangedSlot();
}

void NodeView::Select(const QVector<Node *> &nodes)
{
  if (!graph_) {
    return;
  }

  // Optimization: rather than respond to every single item being selected, ignore the signal and
  //               then handle them all at the end.
  DisconnectSelectionChangedSignal();

  scene_.DeselectAll();

  foreach (Node* n, nodes) {
    NodeViewItem* item = scene_.NodeToUIObject(n);

    item->setSelected(true);
  }

  ConnectSelectionChangedSignal();
  SceneSelectionChangedSlot();
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

void NodeView::SelectBlocks(const QVector<Block *> &blocks)
{
  if (!graph_) {
    return;
  }

  selected_blocks_.append(blocks);

  QueueSelectBlocksInternal();
}

void NodeView::DeselectBlocks(const QVector<Block *> &blocks)
{
  if (!graph_) {
    return;
  }

  // Remove temporary associations
  foreach (Block* b, selected_blocks_) {
    if (!blocks.contains(b)) {
      temporary_association_map_.remove(b);
    }
  }

  // Remove blocks from selected array
  foreach (Block* b, blocks) {
    selected_blocks_.removeOne(b);
  }

  QueueSelectBlocksInternal();
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

  QUndoCommand* command = new QUndoCommand();

  QVector<Node*> pasted_nodes = PasteNodesFromClipboard(static_cast<Sequence*>(graph_), command);

  Core::instance()->undo_stack()->pushIfHasChildren(command);

  if (!pasted_nodes.isEmpty()) {
    foreach (Node* n, pasted_nodes) {
      AssociateNodeWithSelectedBlocks(n);
    }

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

  QUndoCommand* command = new QUndoCommand();

  QVector<Node*> duplicated_nodes = Node::CopyDependencyGraph(selected, command);

  Core::instance()->undo_stack()->pushIfHasChildren(command);

  AttachNodesToCursor(duplicated_nodes);
}

void NodeView::ItemsChanged()
{
  QHash<NodeEdge *, NodeViewEdge *>::const_iterator i;

  for (i=scene_.edge_map().constBegin(); i!=scene_.edge_map().constEnd(); i++) {
    i.value()->Adjust();
  }
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
    // Qt doesn't do this by default for some reason
    if (!(event->modifiers() & Qt::ShiftModifier)) {
      scene_.clearSelection();
    }

    // If there's an item here, select it
    QGraphicsItem* item = itemAt(event->pos());
    if (item) {
      item->setSelected(true);
    }
  }

  super::mousePressEvent(event);
}

void NodeView::mouseMoveEvent(QMouseEvent *event)
{
  if (HandMove(event)) return;

  super::mouseMoveEvent(event);

  if (!attached_items_.isEmpty()) {
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
          drop_input_ = nullptr;

          foreach (NodeParam* param, attached_node->parameters()) {
            if (param->type() == NodeParam::kInput) {
              NodeInput* input = static_cast<NodeInput*>(param);

              if (input->is_connectable()) {
                if (input->data_type() & new_drop_edge->edge()->input()->data_type()) {
                  drop_input_ = input;
                  break;
                } else if (!drop_input_) {
                  drop_input_ = input;
                }
              }
            }
          }

          if (drop_input_) {
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

  if (!attached_items_.isEmpty()) {
    if (attached_items_.size() == 1) {
      Node* dropping_node = attached_items_.first().item->GetNode();

      if (drop_edge_) {
        NodeEdgePtr old_edge = drop_edge_->edge();

        // We have everything we need to place the node in between
        QUndoCommand* command = new QUndoCommand();

        // Remove old edge
        new NodeEdgeRemoveCommand(old_edge, command);

        // Place new edges
        new NodeEdgeAddCommand(old_edge->output(), drop_input_, command);
        new NodeEdgeAddCommand(dropping_node->output(), old_edge->input(), command);

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
  if (event->modifiers() & Qt::ControlModifier) {
    // FIXME: Hardcoded divider (0.001)
    qreal multiplier = 1.0 + (static_cast<qreal>(event->angleDelta().x() + event->angleDelta().y()) * 0.001);

    double test_scale = scale_ * multiplier;

    if (test_scale > 0.1) {
      scale(multiplier, multiplier);
      scale_ = test_scale;
    }
  } else {
    QWidget::wheelEvent(event);
  }
}

void NodeView::SceneSelectionChangedSlot()
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
    // Associate this new node with these blocks (allows it to be visible with them even while it
    // isn't connected to anything)
    AssociateNodeWithSelectedBlocks(new_node);

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
  FilterMode filter = static_cast<FilterMode>(action->data().toInt());

  if (filter_mode_ != filter) {
    filter_mode_ = filter;

    if (filter == kFilterShowAll) {
      // Un-hide all blocks
      foreach (NodeViewItem* item, scene_.item_map()) {
        item->setVisible(true);
      }

      foreach (NodeViewEdge* edge, scene_.edge_map()) {
        edge->setVisible(true);
      }
    }

    ValidateFilter();
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
  connect(&scene_, &QGraphicsScene::selectionChanged, this, &NodeView::SceneSelectionChangedSlot);
}

void NodeView::DisconnectSelectionChangedSignal()
{
  disconnect(&scene_, &QGraphicsScene::selectionChanged, this, &NodeView::SceneSelectionChangedSlot);
}

void NodeView::UpdateBlockFilter()
{
  // Hide all nodes
  foreach (NodeViewItem* item, scene_.item_map()) {
    item->setVisible(false);
  }

  bool first = true;
  QPointF last_bottom_right;

  QVector<Node*> currently_visible;

  foreach (Block* b, selected_blocks_) {
    // Auto-position this node's dependencies
    scene_.ReorganizeFrom(b);

    // Start calculating the bounding rect of this node's deps
    QPointF node_pos = b->GetPosition();
    QRectF anchor(node_pos, node_pos);

    QVector<Node*> deps = b->GetDependencies();

    foreach (Node* d, deps) {
      QPointF dep_pos = d->GetPosition();

      anchor.setLeft(qMin(anchor.left(), dep_pos.x()));
      anchor.setRight(qMax(anchor.right(), dep_pos.x()));
      anchor.setTop(qMin(anchor.top(), dep_pos.y()));
      anchor.setBottom(qMax(anchor.bottom(), dep_pos.y()));

      NodeViewItem* item = scene_.NodeToUIObject(d);
      if (item) {
        item->setVisible(true);
      }
    }

    // Shift the bounding rect in relation to the other bounding rects
    if (first) {
      first = false;
    } else {
      QPointF desired_anchor_pos = last_bottom_right + QPointF(0, 2);

      QPointF necessary_movement = anchor.topRight() - desired_anchor_pos;

      // Calculate the bottom right from which we'll anchor the next rect
      last_bottom_right = anchor.bottomRight() - necessary_movement;

      b->SetPosition(b->GetPosition() - necessary_movement);
      foreach (Node* d, deps) {
        d->SetPosition(d->GetPosition() - necessary_movement);
      }
    }

    // Now that we're done calculating the bounding rect, add this block...
    deps.append(b);

    // ...then add its associations
    deps.append(temporary_association_map_[b]);
    for (auto i=association_map_.begin(); i!=association_map_.end(); i++) {
      if (i.value().contains(b)) {
        deps.append(i.key());
      }
    }

    // And make sure all nodes are shown
    foreach (Node* n, deps) {
      NodeViewItem* item = scene_.NodeToUIObject(n);
      if (item) {
        item->setVisible(true);
      }
    }

    // And lastly, add them all to our currently visible list
    currently_visible.append(deps);
  }

  // Show only edges between those dependencies
  foreach (NodeViewEdge* edge, scene_.edge_map()) {
    edge->setVisible((currently_visible.contains(edge->edge()->input_node())
                      && currently_visible.contains(edge->edge()->output_node())));
  }
}

void NodeView::AssociateNodeWithSelectedBlocks(Node *n)
{
  association_map_.insert(n, selected_blocks_);
  connect(n, &Node::destroyed, this, &NodeView::AssociatedNodeDestroyed, Qt::DirectConnection);
}

void NodeView::DisassociateNode(Node *n, bool remove_from_map)
{
  if (remove_from_map) {
    association_map_.remove(n);
  }

  disconnect(n, &Node::destroyed, this, &NodeView::AssociatedNodeDestroyed);
}

void NodeView::QueueSelectBlocksInternal()
{
  select_blocks_internal_timer_.stop();
  select_blocks_internal_timer_.start();
}

void NodeView::SelectBlocksInternal()
{
  // Block scene signals while our selection is changing a lot
  DisconnectSelectionChangedSignal();

  if (filter_mode_ == kFilterShowSelectedBlocks) {
    UpdateBlockFilter();
  }

  QVector<Node*> nodes;
  nodes.reserve(selected_blocks_.size());

  foreach (Block* b, selected_blocks_) {
    nodes.append(b);
    nodes.append(b->GetDependencies());
  }

  SelectWithDependencies(nodes);

  // Stop blocking signals and send a change signal now that all of our processing is done
  ConnectSelectionChangedSignal();
  SceneSelectionChangedSlot();

  if (!selected_blocks_.isEmpty()) {
    NodeViewItem* item = scene_.NodeToUIObject(selected_blocks_.first());
    if (item) {
      centerOn(item);
    }
  }
}

void NodeView::ValidateFilter()
{
  // Force auto-positioning
  switch (filter_mode_) {
  case kFilterShowAll:
    // NOTE: Assumes Sequence
    scene_.ReorganizeFrom(static_cast<Sequence*>(graph_)->viewer_output());
    break;
  case kFilterShowSelectedBlocks:
    UpdateBlockFilter();
    break;
  }
}

void NodeView::AssociatedNodeDestroyed()
{
  DisassociateNode(static_cast<Node*>(sender()), true);
}

void NodeView::GraphNodeRemoved(Node *node)
{
  if (selected_blocks_.contains(static_cast<Block*>(node))) {
    DeselectBlocks({static_cast<Block*>(node)});
  }
}

void NodeView::GraphEdgeAdded(NodeEdgePtr edge)
{
  Node* input_node = edge->input()->parentNode();

  if (input_node->OutputsTo(static_cast<Sequence*>(graph_)->viewer_output(), true)) {
    auto i = association_map_.begin();

    while (i != association_map_.end()) {
      if (input_node->InputsFrom(i.key(), true)) {
        i = association_map_.erase(i);
      } else {
        i++;
      }
    }
  }

  scene_.AddEdge(edge);

  ValidateFilter();
}

void NodeView::GraphEdgeRemoved(NodeEdgePtr edge)
{
  scene_.RemoveEdge(edge);

  Node* output_node = edge->output_node();

  // Check if this disconnected node still connects to a selected block, in which case do nothing
  foreach (Block* b, selected_blocks_) {
    if (output_node->OutputsTo(b, true)) {
      return;
    }
  }

  QVector<Node*> disconnected_nodes;
  disconnected_nodes.append(output_node);
  disconnected_nodes.append(output_node->GetDependencies());

  if (output_node->OutputsTo(static_cast<Sequence*>(graph_)->viewer_output(), true)) {
    // Check if this disconnected node still has a path to the viewer somewhere else
    foreach (Block* b, selected_blocks_) {
      QVector<Node*>& temp_assocs = temporary_association_map_[b];

      temp_assocs.append(disconnected_nodes);
    }
  } else {
    // Otherwise, we must associate these nodes
    foreach (Node* n, disconnected_nodes) {
      AssociateNodeWithSelectedBlocks(n);
    }
  }
}

OLIVE_NAMESPACE_EXIT
