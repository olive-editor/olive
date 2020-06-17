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

#include "nodetablewidget.h"

#include <QVBoxLayout>

OLIVE_NAMESPACE_ENTER

NodeTableWidget::NodeTableWidget(QWidget* parent) :
  TimeBasedWidget(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  view_ = new NodeTableView();
  layout->addWidget(view_);
}

void NodeTableWidget::SetNodes(const QList<Node *> &nodes)
{
  if (nodes.isEmpty()) {
    view_->clear();
  } else if (nodes.size() == 1) {
    view_->SetNode(nodes.first(), rational());
  } else {
    view_->SetMultipleNodeMessage();
  }
}

OLIVE_NAMESPACE_EXIT
