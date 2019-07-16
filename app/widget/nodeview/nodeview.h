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

#include "node/graph.h"
#include "widget/nodeview/nodeviewedge.h"
#include "widget/nodeview/nodeviewitem.h"

class NodeView : public QGraphicsView
{
  Q_OBJECT
public:
  NodeView(QWidget* parent);

  void SetGraph(NodeGraph* graph);

  static NodeViewItem* NodeToUIObject(QGraphicsScene* scene, Node* n);
  static NodeViewEdge* EdgeToUIObject(QGraphicsScene* scene, NodeEdgePtr n);

private:
  NodeGraph* graph_;

  QGraphicsScene scene_;

private slots:
  void AddEdge(NodeEdgePtr edge);

  void RemoveEdge(NodeEdgePtr edge);

  void ItemsChanged();

};

#endif // NODEVIEW_H
