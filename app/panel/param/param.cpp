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

#include "param.h"

#include "window/mainwindow/mainwindow.h"

namespace olive {

ParamPanel::ParamPanel() :
  TimeBasedPanel(QStringLiteral("ParamPanel"))
{
  NodeParamView* view = new NodeParamView(this);
  connect(view, &NodeParamView::FocusedNodeChanged, this, &ParamPanel::FocusedNodeChanged);
  connect(view, &NodeParamView::SelectedNodesChanged, this, &ParamPanel::SelectedNodesChanged);
  connect(view, &NodeParamView::RequestViewerToStartEditingText, this, &ParamPanel::RequestViewerToStartEditingText);
  connect(this, &ParamPanel::shown, view, &NodeParamView::UpdateElementY);
  SetTimeBasedWidget(view);

  Retranslate();
}

void ParamPanel::DeleteSelected()
{
  static_cast<NodeParamView*>(GetTimeBasedWidget())->DeleteSelected();
}

void ParamPanel::SelectAll()
{
  static_cast<NodeParamView*>(GetTimeBasedWidget())->SelectAll();
}

void ParamPanel::DeselectAll()
{
  static_cast<NodeParamView*>(GetTimeBasedWidget())->DeselectAll();
}

void ParamPanel::SetContexts(const QVector<Node *> &contexts)
{
  static_cast<NodeParamView*>(GetTimeBasedWidget())->SetContexts(contexts);
}

void ParamPanel::Retranslate()
{
  SetTitle(tr("Parameter Editor"));
}

}
