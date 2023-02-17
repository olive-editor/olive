/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "node.h"

namespace olive {

NodePanel::NodePanel() :
  PanelWidget(QStringLiteral("NodePanel"))
{
  node_widget_ = new NodeWidget(this);
  connect(this, &NodePanel::shown, node_widget_->view(), &NodeView::CenterOnItemsBoundingRect);

  connect(node_widget_->view(), &NodeView::NodesSelected, this, &NodePanel::NodesSelected);
  connect(node_widget_->view(), &NodeView::NodesDeselected, this, &NodePanel::NodesDeselected);
  connect(node_widget_->view(), &NodeView::NodeSelectionChanged, this, &NodePanel::NodeSelectionChanged);
  connect(node_widget_->view(), &NodeView::NodeSelectionChangedWithContexts, this, &NodePanel::NodeSelectionChangedWithContexts);
  connect(node_widget_->view(), &NodeView::NodeGroupOpened, this, &NodePanel::NodeGroupOpened);
  connect(node_widget_->view(), &NodeView::NodeGroupClosed, this, &NodePanel::NodeGroupClosed);

  // Set it as the main widget of this panel
  SetWidgetWithPadding(node_widget_);

  // Set strings
  Retranslate();
}

}
