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

#include "param.h"

#include "window/mainwindow/mainwindow.h"

OLIVE_NAMESPACE_ENTER

ParamPanel::ParamPanel(QWidget* parent) :
  TimeBasedPanel(QStringLiteral("ParamPanel"), parent)
{
  NodeParamView* view = new NodeParamView();
  connect(view, &NodeParamView::RequestSelectNode, this, &ParamPanel::RequestSelectNode);
  connect(view, &NodeParamView::NodeOrderChanged, this, &ParamPanel::NodeOrderChanged);
  SetTimeBasedWidget(view);

  Retranslate();
}

void ParamPanel::SelectNodes(const QList<Node *> &nodes)
{
  static_cast<NodeParamView*>(GetTimeBasedWidget())->SelectNodes(nodes);

  Retranslate();
}

void ParamPanel::DeselectNodes(const QList<Node *> &nodes)
{
  static_cast<NodeParamView*>(GetTimeBasedWidget())->DeselectNodes(nodes);

  Retranslate();
}

void ParamPanel::DeleteSelected()
{
  static_cast<NodeParamView*>(GetTimeBasedWidget())->DeleteSelected();
}

void ParamPanel::Retranslate()
{
  SetTitle(tr("Parameter Editor"));

  NodeParamView* view = static_cast<NodeParamView*>(GetTimeBasedWidget());

  if (view->GetItemMap().isEmpty()) {
    SetSubtitle(tr("(none)"));
  } else if (view->GetItemMap().size() == 1) {
    SetSubtitle(view->GetItemMap().firstKey()->Name());
  } else {
    SetSubtitle(tr("(multiple)"));
  }
}

OLIVE_NAMESPACE_EXIT
