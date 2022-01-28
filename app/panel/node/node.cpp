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

#include "node.h"

namespace olive {

NodePanel::NodePanel(QWidget *parent) :
  PanelWidget(QStringLiteral("NodePanel"), parent)
{
  node_widget_ = new NodeWidget();
  connect(this, &NodePanel::visibilityChanged, node_widget_->view(), &NodeView::CenterOnItemsBoundingRect);

  // Connect node view signals to this panel - MAY REMOVE
  connect(node_widget_->view(), &NodeView::NodesSelected, this, &NodePanel::NodesSelected);
  connect(node_widget_->view(), &NodeView::NodesDeselected, this, &NodePanel::NodesDeselected);
  connect(node_widget_->view(), &NodeView::NodeGroupOpenRequested, this, &NodePanel::NodeGroupOpenRequested);

  // Set it as the main widget of this panel
  SetWidgetWithPadding(node_widget_);

  // Set strings
  Retranslate();
}

}
