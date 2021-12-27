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
#include "undo/undostack.h"

namespace olive {

class NodeViewScene : public QGraphicsScene
{
  Q_OBJECT
public:
  NodeViewScene(QObject *parent = nullptr);

  void SelectAll();
  void DeselectAll();

  QVector<NodeViewItem*> GetSelectedItems() const;

  const QHash<Node*, NodeViewContext*> &context_map() const
  {
    return context_map_;
  }

  Qt::Orientation GetFlowOrientation() const;

  NodeViewCommon::FlowDirection GetFlowDirection() const
  {
    return direction_;
  }

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
  QHash<Node*, NodeViewContext*> context_map_;

  NodeGraph* graph_;

  NodeViewCommon::FlowDirection direction_;

  bool curved_edges_;

};

}

#endif // NODEVIEWSCENE_H
