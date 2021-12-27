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

#ifndef NODEWIDGET_H
#define NODEWIDGET_H

#include <QWidget>

#include "nodeview.h"
#include "nodeviewtoolbar.h"

namespace olive {

class NodeWidget : public QWidget
{
  Q_OBJECT
public:
  NodeWidget(QWidget *parent = nullptr);

  NodeView *view() const
  {
    return node_view_;
  }

  void SetContexts(const QVector<Node*> &nodes)
  {
    node_view_->SetContexts(nodes);
    toolbar_->setEnabled(!nodes.isEmpty());
  }

private:
  NodeView *node_view_;

  NodeViewToolBar *toolbar_;

};

}

#endif // NODEWIDGET_H
