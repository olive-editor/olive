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

#ifndef NODEVIEW_H
#define NODEVIEW_H

#include <QGraphicsView>
#include <QTimer>

#include "core.h"
#include "node/graph.h"
#include "node/nodecopypaste.h"
#include "nodeviewedge.h"
#include "nodeviewcontext.h"
#include "nodeviewminimap.h"
#include "nodeviewscene.h"
#include "widget/handmovableview/handmovableview.h"
#include "widget/menu/menu.h"

namespace olive {

/**
 * @brief A widget for viewing and editing node graphs
 *
 * This widget takes a NodeGraph object and constructs a QGraphicsScene representing its data, viewing and allowing
 * the user to make modifications to it.
 */
class NodeView : public HandMovableView, public NodeCopyPasteService
{
  Q_OBJECT
public:
  NodeView(QWidget* parent = nullptr);

  virtual ~NodeView() override;

  void SetContexts(const QVector<Node *> &nodes);

  const QVector<Node*> &GetContexts() const
  {
    return contexts_;
  }

  void CloseContextsBelongingToProject(Project *project);

  void ClearGraph();

  /**
   * @brief Delete selected nodes from graph (user-friendly/undoable)
   */
  void DeleteSelected();

  void SelectAll();
  void DeselectAll();

  void Select(const QVector<Node *> &nodes, bool center_view_on_item);

  void CopySelected(bool cut);
  void Paste();

  void Duplicate();

  void SetColorLabel(int index);

  void ZoomIn();

  void ZoomOut();

  const QVector<Node*> &GetCurrentContexts() const
  {
    return contexts_;
  }

public slots:
  void SetMiniMapEnabled(bool e)
  {
    minimap_->setVisible(e);
  }

  void ShowAddMenu()
  {
    Menu *m = CreateAddMenu(nullptr);
    m->exec(QCursor::pos());
    delete m;
  }

  void CenterOnItemsBoundingRect();

  void CenterOnNode(olive::Node *n);

signals:
  void NodesSelected(const QVector<Node*>& nodes);

  void NodesDeselected(const QVector<Node*>& nodes);

  void NodeGroupOpenRequested(NodeGroup *group);

protected:
  virtual void keyPressEvent(QKeyEvent *event) override;

  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* event) override;

  virtual void resizeEvent(QResizeEvent *event) override;

  virtual void ZoomIntoCursorPosition(QWheelEvent *event, double multiplier, const QPointF &cursor_pos) override;

  virtual bool event(QEvent *event) override;

  virtual bool eventFilter(QObject *object, QEvent *event) override;

  virtual void CopyNodesToClipboardInternal(QXmlStreamWriter *writer, const QVector<Node*> &nodes, void* userdata) override;
  virtual void PasteNodesFromClipboardInternal(QXmlStreamReader *reader, XMLNodeData &xml_node_data, void* userdata) override;

  virtual void changeEvent(QEvent *e) override;

private:
  void DetachItemsFromCursor(bool delete_nodes_too = true);

  void SetFlowDirection(NodeViewCommon::FlowDirection dir);

  void MoveAttachedNodesToCursor(const QPoint &p);

  void ConnectSelectionChangedSignal();
  void DisconnectSelectionChangedSignal();

  void ZoomFromKeyboard(double multiplier);

  void ClearCreateEdgeInputIfNecessary();

  QPointF GetEstimatedPositionForContext(NodeViewItem *item, Node *context) const;

  NodeViewItem *GetAssumedItemForSelectedNode(Node *node);
  Node::Position GetAssumedPositionForSelectedNode(Node *node);

  Menu *CreateAddMenu(Menu *parent);

  void PositionNewEdge(const QPoint &pos);

  void PasteNodesInternal(const QVector<Node*> &duplicate_nodes = QVector<Node *>());

  void AddContext(Node *n);

  void RemoveContext(Node *n);

  bool IsItemAttachedToCursor(NodeViewItem *item) const;

  void ExpandItem(NodeViewItem *item);

  void CollapseItem(NodeViewItem *item);

  void EndEdgeDrag(bool cancel = false);

  NodeViewMiniMap *minimap_;

  struct AttachedItem {
    NodeViewItem* item;
    Node *node;
    QPointF original_pos;
  };

  void SetAttachedItems(const QVector<AttachedItem> &items);
  QVector<AttachedItem> attached_items_;

  NodeViewEdge* drop_edge_;
  NodeInput drop_input_;

  NodeViewEdge* create_edge_;
  NodeViewItem* create_edge_output_item_;
  NodeViewItem* create_edge_input_item_;
  NodeInput create_edge_input_;
  bool create_edge_already_exists_;
  bool create_edge_from_output_;

  QVector<NodeViewItem*> create_edge_expanded_items_;

  NodeViewScene scene_;

  QVector<Node*> selected_nodes_;

  QVector<Node*> contexts_;
  QVector<Node*> last_set_filter_nodes_;
  QMap<Node*, QPointF> context_offsets_;

  QMap<NodeViewItem*, QPointF> dragging_items_;

  double scale_;

  static const double kMinimumScale;

private slots:
  /**
   * @brief Receiver for when the scene's selected items change
   */
  void UpdateSelectionCache();

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
   * @brief Opens the selected node in a Viewer
   */
  void OpenSelectedNodeInViewer();

  void UpdateSceneBoundingRect();

  void RepositionMiniMap();

  void UpdateViewportOnMiniMap();

  void MoveToScenePoint(const QPointF &pos);

  void NodeRemovedFromGraph();

  void GroupNodes();

  void UngroupNodes();

  void ShowNodeProperties();

  void LabelSelectedNodes();

  void ItemAboutToBeDeleted(NodeViewItem *item);

};

}

#endif // NODEVIEW_H
