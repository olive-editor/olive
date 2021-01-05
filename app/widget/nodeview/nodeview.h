/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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
#include "node/nodecopypaste.h"
#include "widget/handmovableview/handmovableview.h"

namespace olive {

class NodeView : public HandMovableView, public NodeCopyPasteService
{
  Q_OBJECT
public:
  NodeView(QWidget* parent);

  /**
   * @brief Sets the graph to view
   */
  void SetGraph(NodeGraph* graph){}

  /**
   * @brief Delete selected nodes from graph (user-friendly/undoable)
   */
  void DeleteSelected(){}

  void SelectAll(){}
  void DeselectAll(){}

  void Select(const QVector<Node*>& nodes){}
  void SelectWithDependencies(QVector<Node *> nodes){}

  void CopySelected(bool cut){}
  void Paste(){}

  void Duplicate(){}

  void SelectBlocks(const QVector<Block*>& blocks){}

  void DeselectBlocks(const QVector<Block*>& blocks){}

private:
  NodeGraph* graph_;

};

}

#endif // NODEVIEW_H
