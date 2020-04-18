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

OLIVE_NAMESPACE_ENTER

NodePanel::NodePanel(QWidget *parent) :
  PanelWidget(QStringLiteral("NodePanel"), parent)
{
  // Create NodeView widget
  node_view_ = new NodeView(this);

  // Connect node view signals to this panel
  connect(node_view_, SIGNAL(SelectionChanged(QList<Node*>)), this, SIGNAL(SelectionChanged(QList<Node*>)));

  // Set it as the main widget of this panel
  setWidget(node_view_);

  // Set strings
  Retranslate();
}

void NodePanel::SetGraph(NodeGraph *graph)
{
  node_view_->SetGraph(graph);
}

void NodePanel::SelectAll()
{
  node_view_->SelectAll();
}

void NodePanel::DeselectAll()
{
  node_view_->DeselectAll();
}

void NodePanel::DeleteSelected()
{
  node_view_->DeleteSelected();
}

void NodePanel::CutSelected()
{
  node_view_->CopySelected(true);
}

void NodePanel::CopySelected()
{
  node_view_->CopySelected(false);
}

void NodePanel::Paste()
{
  node_view_->Paste();
}

void NodePanel::Select(const QList<Node *> &nodes)
{
  node_view_->Select(nodes);
}

void NodePanel::SelectWithDependencies(const QList<Node *> &nodes)
{
  node_view_->SelectWithDependencies(nodes);
}

void NodePanel::Retranslate()
{
  SetTitle(tr("Node Editor"));
}

OLIVE_NAMESPACE_EXIT
