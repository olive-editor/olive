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

#ifndef NODEVIEW_H
#define NODEVIEW_H

#include <QGraphicsView>
#include <QTimer>

#include "node/graph.h"
#include "nodeviewscene.h"
#include "widget/timelinewidget/view/handmovableview.h"
#include "widget/nodecopypaste/nodecopypaste.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A widget for viewing and editing node graphs
 *
 * This widget takes a NodeGraph object and constructs a QGraphicsScene representing its data, viewing and allowing
 * the user to make modifications to it.
 */
class NodeView : public HandMovableView, public NodeCopyPasteWidget
{
  Q_OBJECT
public:
  NodeView(QWidget* parent);

  virtual ~NodeView() override;

  /**
   * @brief Sets the graph to view
   */
  void SetGraph(NodeGraph* graph);

  /**
   * @brief Delete selected nodes from graph (user-friendly/undoable)
   */
  void DeleteSelected();

  void SelectAll();
  void DeselectAll();

  void Select(const QVector<Node*>& nodes);
  void SelectWithDependencies(QVector<Node *> nodes);

  void CopySelected(bool cut);
  void Paste();

  void Duplicate();

  void SelectBlocks(const QVector<Block*>& blocks);

  void DeselectBlocks(const QVector<Block*>& blocks);

signals:
  void NodesSelected(const QVector<Node*>& nodes);

  void NodesDeselected(const QVector<Node*>& nodes);

protected:
  virtual void keyPressEvent(QKeyEvent *event) override;

  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;

  virtual void wheelEvent(QWheelEvent* event) override;

private:
  void AttachNodesToCursor(const QVector<Node *> &nodes);

  void AttachItemsToCursor(const QVector<NodeViewItem *> &items);

  void DetachItemsFromCursor();

  void SetFlowDirection(NodeViewCommon::FlowDirection dir);

  void MoveAttachedNodesToCursor(const QPoint &p);

  void ConnectSelectionChangedSignal();
  void DisconnectSelectionChangedSignal();

  void UpdateBlockFilter();

  void AssociateNodeWithSelectedBlocks(Node* n);
  void DisassociateNode(Node* n, bool remove_from_map);

  void QueueSelectBlocksInternal();

  QTimer select_blocks_internal_timer_;

  NodeGraph* graph_;

  struct AttachedItem {
    NodeViewItem* item;
    QPointF original_pos;
  };

  QList<AttachedItem> attached_items_;

  NodeViewEdge* drop_edge_;
  NodeInput* drop_input_;

  NodeViewScene scene_;

  QVector<Node*> selected_nodes_;

  QVector<Block*> selected_blocks_;

  QHash<Node*, QVector<Block*> > association_map_;

  QHash<Block*, QVector<Node*> > temporary_association_map_;

  enum FilterMode {
    kFilterShowAll,
    kFilterShowSelectedBlocks
  };

  FilterMode filter_mode_;

  double scale_;

private slots:
  void ValidateFilter();

  void AssociatedNodeDestroyed();

  void GraphNodeRemoved(Node* node);

  void GraphEdgeAdded(NodeEdgePtr edge);

  void GraphEdgeRemoved(NodeEdgePtr edge);

  /**
   * @brief Internal function triggered when any change is signalled from the QGraphicsScene
   *
   * Current primary function is to inform all NodeViewEdges to re-adjust in case any Nodes have moved
   */
  void ItemsChanged();

  /**
   * @brief Receiver for when the scene's selected items change
   */
  void SceneSelectionChangedSlot();

  /**
   * @brief Receiver for when the user right clicks (or otherwise requests a context menu)
   */
  void ShowContextMenu(const QPoint &pos);

  /**
   * @brief Receiver for when the user requests a new node from the add menu
   */
  void CreateNodeSlot(QAction* action);

  /**
   * @brief Receiver for setting the direction from the context menu
   */
  void ContextMenuSetDirection(QAction* action);

  /**
   * @brief Receiver for auto-position descendents menu action
   */
  void AutoPositionDescendents();

  /**
   * @brief Receiver for the user changing the filter
   */
  void ContextMenuFilterChanged(QAction* action);

  void SelectBlocksInternal();

};

OLIVE_NAMESPACE_EXIT

#endif // NODEVIEW_H
