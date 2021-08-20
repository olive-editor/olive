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

#include "node/graph.h"
#include "node/nodecopypaste.h"
#include "nodeviewedge.h"
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
  NodeView(QWidget* parent);

  virtual ~NodeView() override;

  NodeGraph* GetGraph() const
  {
    return graph_;
  }

  void SetGraph(NodeGraph *graph, const QVector<Node *> &nodes);

  void ClearGraph();

  /**
   * @brief Delete selected nodes from graph (user-friendly/undoable)
   */
  void DeleteSelected();

  void SelectAll();
  void DeselectAll();

  void Select(QVector<Node *> nodes, bool center_view_on_item);
  void SelectWithDependencies(QVector<Node *> nodes, bool center_view_on_item);

  void CopySelected(bool cut);
  void Paste();

  void Duplicate();

  void SetColorLabel(int index);

  void ZoomIn();

  void ZoomOut();

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

signals:
  void NodesSelected(const QVector<Node*>& nodes);

  void NodesDeselected(const QVector<Node*>& nodes);

protected:
  virtual void keyPressEvent(QKeyEvent *event) override;

  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;

  virtual void resizeEvent(QResizeEvent *event) override;

  virtual void ZoomIntoCursorPosition(QWheelEvent *event, double multiplier, const QPointF &cursor_pos) override;

  virtual bool event(QEvent *event) override;

  virtual bool eventFilter(QObject *object, QEvent *event) override;

  virtual void CopyNodesToClipboardInternal(QXmlStreamWriter *writer, const QVector<Node*> &nodes, void* userdata) override;
  virtual void PasteNodesFromClipboardInternal(QXmlStreamReader *reader, XMLNodeData &xml_node_data, void* userdata) override;

private:
  void AttachNodesToCursor(const QVector<Node *> &nodes);

  void AttachItemsToCursor(const QVector<NodeViewItem *> &items);

  void DetachItemsFromCursor();

  void SetFlowDirection(NodeViewCommon::FlowDirection dir);

  void MoveAttachedNodesToCursor(const QPoint &p);

  void ConnectSelectionChangedSignal();
  void DisconnectSelectionChangedSignal();

  void ZoomFromKeyboard(double multiplier);

  bool DetermineIfNodeIsFloatingInContext(Node *node, Node *context, Node *source, const Node::OutputConnections &removed_edges, const Node::OutputConnection &added_edge);
  void UpdateContextsFromEdgeRemove(MultiUndoCommand *command, const Node::OutputConnections &remove_edges);
  void UpdateContextsFromEdgeAdd(MultiUndoCommand *command, const Node::OutputConnection &added_edge, const Node::OutputConnections &removed_edges = Node::OutputConnections());
  void RecursivelyAddNodeToContext(MultiUndoCommand *command, Node *node, Node *context);
  void RecursivelyRemoveFloatingNodeFromContext(MultiUndoCommand *command, Node *node, Node *context, Node *source, const Node::OutputConnections &removed_edges, const Node::OutputConnection &added_edge, bool prevent_removing);

  QPointF GetEstimatedPositionForContext(NodeViewItem *item, Node *context) const;

  Menu *CreateAddMenu(Menu *parent);

  void CreateNewEdge(NodeViewItem *output_item, const QPoint &mouse_pos);

  void PositionNewEdge(const QPoint &pos);

  NodeViewItem *UpdateNodeItem(Node *node, bool ignore_own_context = false);

  void PasteNodesInternal(const QVector<Node*> &duplicate_nodes = QVector<Node *>());

  class NodeViewAttachNodesToCursor : public UndoCommand
  {
  public:
    NodeViewAttachNodesToCursor(NodeView* view, const QVector<Node*>& nodes);

    virtual Project * GetRelevantProject() const override;

  protected:
    virtual void redo() override;

    virtual void undo() override;

  private:
    NodeView* view_;

    QVector<Node*> nodes_;

  };

  NodeViewMiniMap *minimap_;

  NodeGraph* graph_;

  struct AttachedItem {
    NodeViewItem* item;
    QPointF original_pos;
  };

  class NodeViewItemPreventRemovingCommand : public UndoCommand
  {
  public:
    NodeViewItemPreventRemovingCommand(NodeView *view, Node *node, bool prevent_removing) :
      view_(view),
      node_(node),
      new_prevent_removing_(prevent_removing)
    {}

    virtual Project * GetRelevantProject() const override
    {
      return node_->project();
    }

  protected:
    virtual void redo() override;

    virtual void undo() override;

  private:
    NodeView *view_;
    Node *node_;
    bool new_prevent_removing_;
    bool old_prevent_removing_;

  };

  QList<AttachedItem> attached_items_;

  NodeViewEdge* drop_edge_;
  NodeInput drop_input_;

  NodeViewEdge* create_edge_;
  NodeViewItem* create_edge_src_;
  QString create_edge_src_output_;
  NodeViewItem* create_edge_dst_;
  NodeInput create_edge_dst_input_;
  bool create_edge_dst_temp_expanded_;

  NodeViewScene scene_;

  MultiUndoCommand* paste_command_;

  QVector<Node*> selected_nodes_;

  enum FilterMode {
    kFilterShowAll,
    kFilterShowSelective
  };

  struct Position {
    Node *node;
    QPointF original_item_pos;
  };

  QMap<NodeViewItem *, Position> positions_;

  FilterMode filter_mode_;

  QVector<Node*> filter_nodes_;
  QVector<Node*> last_set_filter_nodes_;
  QMap<Node*, QPointF> context_offsets_;

  double scale_;

  bool create_edge_already_exists_;

  bool queue_reposition_contexts_;

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
   * @brief Receiver for the user changing the filter
   */
  void ContextMenuFilterChanged(QAction* action);

  /**
   * @brief Opens the selected node in a Viewer
   */
  void OpenSelectedNodeInViewer();

  //void AddNode(Node *node);
  void RemoveNode(Node *node);
  void AddEdge(const NodeOutput& output, const NodeInput& input);
  void RemoveEdge(const NodeOutput& output, const NodeInput& input);

  void AddNodePosition(Node *node, Node *relative);
  void RemoveNodePosition(Node *node, Node *relative);

  void UpdateSceneBoundingRect();

  void CenterOnItemsBoundingRect();

  void RepositionMiniMap();

  void UpdateViewportOnMiniMap();

  void MoveToScenePoint(const QPointF &pos);

  void RepositionContexts();

};

}

#endif // NODEVIEW_H
