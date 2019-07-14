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

#include "node.h"

NodePanel::NodePanel(QWidget *parent) :
  PanelWidget(parent)
{
  // Create NodeView widget
  node_view_ = new NodeView(this);

  // Set it as the main widget of this panel
  setWidget(node_view_);

  // Set strings
  Retranslate();
}

void NodePanel::SetGraph(NodeGraph *graph)
{
  SetSubtitle(graph->name());

  node_view_->SetGraph(graph);
}

void NodePanel::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
  QDockWidget::changeEvent(e);
}

void NodePanel::Retranslate()
{
  SetTitle(tr("Node Editor"));
  SetSubtitle(tr("(none)"));
}
