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

#ifndef NODEVIEWSCENE_H
#define NODEVIEWSCENE_H

#include <QGraphicsScene>
#include <QTimer>

#include "node/graph.h"
#include "nodeviewcontext.h"
#include "nodeviewedge.h"
#include "nodeviewitem.h"

namespace olive {

class NodeViewScene : public QGraphicsScene
{
  Q_OBJECT
public:
  NodeViewScene(QObject *parent = nullptr);

  void clear();

  void SelectAll();
  void DeselectAll();

  void DeleteSelected();

  /**
   * @brief Retrieve the graphical widget corresponding to a specific Node
   *
   * In situations where you know what Node you're working with but need the UI object (e.g. for positioning), this
   * static function will retrieve the NodeViewItem (Node UI representation) connected to this Node in a certain
   * QGraphicsScene. This can be called from any other UI object, since it'll have a reference to the QGraphicsScene
   * through QGraphicsItem::scene().
   *
   * If the scene does not contain a widget for this node (usually meaning the node's graph is not the active graph
   * in this view/scene), this function returns nullptr.
   */
  NodeViewItem* NodeToUIObject(Node* n);

  QVector<Node *> GetSelectedNodes() const;
  QVector<NodeViewItem*> GetSelectedItems() const;

  const QHash<Node*, NodeViewContext*> &context_map() const
  {
    return context_map_;
  }

  const QHash<Node*, NodeViewItem*>& item_map() const
  {
    return item_map_;
  }

  Qt::Orientation GetFlowOrientation() const;

  NodeViewCommon::FlowDirection GetFlowDirection() const;
  void SetFlowDirection(NodeViewCommon::FlowDirection direction);

  bool GetEdgesAreCurved() const
  {
    return curved_edges_;
  }

public slots:
  NodeViewContext *AddContext(Node *node);
  void RemoveContext(Node *node);

  /**
   * @brief Set whether edges in this scene should be curved or not
   */
  void SetEdgesAreCurved(bool curved);

private:
  static int DetermineWeight(Node* n);

  QHash<Node*, NodeViewContext*> context_map_;

  QHash<Node*, NodeViewItem*> item_map_;

  NodeGraph* graph_;

  NodeViewCommon::FlowDirection direction_;

  bool curved_edges_;

};

}

#endif // NODEVIEWSCENE_H
