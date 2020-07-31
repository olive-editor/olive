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
  connect(view, &NodeParamView::InputDoubleClicked, this, &ParamPanel::CreateCurvePanel);
  connect(view, &NodeParamView::RequestSelectNode, this, &ParamPanel::RequestSelectNode);
  //connect(view, &NodeParamView::FoundGizmos, this, &ParamPanel::FoundGizmos);
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

void ParamPanel::SetTimestamp(const int64_t &timestamp)
{
  TimeBasedPanel::SetTimestamp(timestamp);

  // Ensure all CurvePanels are updated with this time too
  ParamViewTimeChanged(timestamp);
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

void ParamPanel::CreateCurvePanel(NodeInput *input)
{
  if (!input->is_keyframable()) {
    return;
  }

  CurvePanel* panel = open_curve_panels_.value(input);

  if (panel) {
    panel->raise();
    return;
  }

  NodeParamView* view = static_cast<NodeParamView*>(GetTimeBasedWidget());

  panel = Core::instance()->main_window()->AppendCurvePanel();

  panel->ConnectViewerNode(view->GetConnectedNode());
  panel->SetTimestamp(view->GetTimestamp());
  panel->SetInput(input);

  connect(view, &NodeParamView::TimeChanged, this, &ParamPanel::ParamViewTimeChanged);
  connect(panel, &CurvePanel::TimeChanged, this, &ParamPanel::CurvePanelTimeChanged);
  connect(panel, &CurvePanel::CloseRequested, this, &ParamPanel::ClosingCurvePanel);

  open_curve_panels_.insert(input, panel);
}

void ParamPanel::ClosingCurvePanel()
{
  CurvePanel* panel = static_cast<CurvePanel*>(sender());
  open_curve_panels_.remove(panel->GetInput());
}

void ParamPanel::ParamViewTimeChanged(const int64_t &time)
{
  // Ensure all CurvePanels are updated with this time too
  QHash<NodeInput*, CurvePanel*>::const_iterator i;

  for (i=open_curve_panels_.begin(); i!=open_curve_panels_.end(); i++) {
    // If connected viewers are the same, set the timestamp
    if (i.value()->GetConnectedViewer() == GetConnectedViewer()) {
      i.value()->SetTimestamp(time);
    }
  }
}

void ParamPanel::CurvePanelTimeChanged(const int64_t &time)
{
  GetTimeBasedWidget()->SetTimestamp(time);
  emit GetTimeBasedWidget()->TimeChanged(time);

  CurvePanel* src = static_cast<CurvePanel*>(sender());

  // Ensure all CurvePanels are updated with this time too
  QHash<NodeInput*, CurvePanel*>::const_iterator i;

  for (i=open_curve_panels_.begin(); i!=open_curve_panels_.end(); i++) {
    // If connected viewers are the same and the panel isn't the source, set the timestamp
    if (i.value() != src && i.value()->GetConnectedViewer() == src->GetConnectedViewer()) {
      i.value()->SetTimestamp(time);
    }
  }
}

OLIVE_NAMESPACE_EXIT
