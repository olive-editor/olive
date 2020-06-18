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
  TimeBasedWidget(parent),
  node_(nullptr)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  view_ = new NodeTableView();
  layout->addWidget(view_);
}

void NodeTableWidget::SetNodes(const QList<Node *> &nodes)
{
  node_ = nullptr;

  if (nodes.isEmpty()) {
    view_->clear();
  } else if (nodes.size() == 1) {
    node_ = nodes.first();

    ViewerOutput* viewer = node_->FindOutputNode<ViewerOutput>();
    if (viewer) {
      qDebug() << "Found timebase";
      SetTimebase(viewer->video_params().time_base());
    }

    UpdateView();
  } else {
    view_->SetMultipleNodeMessage();
  }
}

void NodeTableWidget::TimeChangedEvent(const int64_t &)
{
  UpdateView();
}

void NodeTableWidget::UpdateView()
{
  if (node_) {
    view_->SetNode(node_, GetTime());
  }
}

OLIVE_NAMESPACE_EXIT
