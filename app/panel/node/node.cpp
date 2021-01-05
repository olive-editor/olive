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

#include "node.h"

namespace olive {

NodePanel::NodePanel(QWidget *parent) :
  PanelWidget(QStringLiteral("NodePanel"), parent)
{
  // Create NodeView widget
  node_view_ = new NodeView(this);

  // Connect node view signals to this panel
  //connect(node_view_, &NodeView::NodesSelected, this, &NodePanel::NodesSelected);
  //connect(node_view_, &NodeView::NodesDeselected, this, &NodePanel::NodesDeselected);

  // Set it as the main widget of this panel
  SetWidgetWithPadding(node_view_);

  // Set strings
  Retranslate();
}

}
