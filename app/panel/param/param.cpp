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

ParamPanel::ParamPanel(QWidget* parent) :
  TimeBasedPanel(parent)
{
  // FIXME: This won't work if there's ever more than one of this panel
  setObjectName("ParamPanel");

  NodeParamView* view = new NodeParamView();
  connect(view, &NodeParamView::SelectedInputChanged, this, &ParamPanel::SelectedInputChanged);
  SetTimeBasedWidget(view);

  Retranslate();
}

void ParamPanel::SetNodes(QList<Node *> nodes)
{
  static_cast<NodeParamView*>(GetTimeBasedWidget())->SetNodes(nodes);

  Retranslate();
}

void ParamPanel::Retranslate()
{
  SetTitle(tr("Parameter Editor"));

  NodeParamView* view = static_cast<NodeParamView*>(GetTimeBasedWidget());

  if (view->nodes().isEmpty()) {
    SetSubtitle(tr("(none)"));
  } else if (view->nodes().size() == 1) {
    SetSubtitle(view->nodes().first()->Name());
  } else {
    SetSubtitle(tr("(multiple)"));
  }
}
