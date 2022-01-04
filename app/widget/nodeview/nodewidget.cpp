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

#include "nodewidget.h"

#include <QVBoxLayout>

namespace olive {

NodeWidget::NodeWidget(QWidget *parent) :
  QWidget(parent)
{
  QVBoxLayout *outer_layout = new QVBoxLayout(this);
  outer_layout->setMargin(0);

  toolbar_ = new NodeViewToolBar();
  outer_layout->addWidget(toolbar_);

  // Create NodeView widget
  node_view_ = new NodeView(this);
  outer_layout->addWidget(node_view_);

  // Connect toolbar to NodeView
  connect(toolbar_, &NodeViewToolBar::MiniMapEnabledToggled, node_view_, &NodeView::SetMiniMapEnabled);
  connect(toolbar_, &NodeViewToolBar::AddNodeClicked, node_view_, &NodeView::ShowAddMenu);

  // Set defaults
  toolbar_->SetMiniMapEnabled(true);
  node_view_->SetMiniMapEnabled(true);

  setSizePolicy(node_view_->sizePolicy());
}

}
