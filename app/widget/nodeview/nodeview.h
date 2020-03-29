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

/**
 * @brief A widget for viewing and editing node graphs
 *
 * This widget takes a NodeGraph object and constructs a QGraphicsScene representing its data, viewing and allowing
 * the user to make modifications to it.
 */
class NodeView : public QGraphicsView
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

  void Select(const QList<Node*>& nodes);
  void SelectWithDependencies(QList<Node *> nodes);

  void CopySelected(bool cut);
  void Paste();

signals:
  /**
   * @brief Signal emitted when the selected nodes have changed
   */
  void SelectionChanged(QList<Node*> selected_nodes);

protected:
  virtual void keyPressEvent(QKeyEvent *event) override;

  virtual void mousePressEvent(QMouseEvent *event) override;

  virtual void mouseMoveEvent(QMouseEvent *event) override;

  virtual void wheelEvent(QWheelEvent* event) override;

private:
  void PlaceNode(NodeViewItem* n, const QPointF& pos);

  void AttachItemToCursor(NodeViewItem* item);

  void DetachItemFromCursor();

  NodeGraph* graph_;

  NodeViewItem* attached_item_;

  NodeViewEdge* drop_edge_;
  NodeInput* drop_compatible_input_;

  NodeViewScene scene_;

private slots:
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

};

#endif // NODEVIEW_H
