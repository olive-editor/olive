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
  connect(view, &NodeParamView::TimeTargetChanged, this, &ParamPanel::TimeTargetChanged);
  connect(view, &NodeParamView::RequestSelectNode, this, &ParamPanel::RequestSelectNode);
  connect(view, &NodeParamView::OpenedNode, this, &ParamPanel::OpeningNode);
  connect(view, &NodeParamView::ClosedNode, this, &ParamPanel::ClosingNode);
  SetTimeBasedWidget(view);

  Retranslate();
}

void ParamPanel::SetNodes(QList<Node *> nodes)
{
  static_cast<NodeParamView*>(GetTimeBasedWidget())->SetNodes(nodes);

  Retranslate();
}

void ParamPanel::SetTimestamp(const int64_t &timestamp)
{
  TimeBasedPanel::SetTimestamp(timestamp);

  // Ensure all CurvePanels are updated with this time too
  QHash<NodeInput*, CurvePanel*>::const_iterator i;

  for (i=open_curve_panels_.begin(); i!=open_curve_panels_.end(); i++) {
    if (i.value() && i.value() != sender()) {
      i.value()->SetTimestamp(timestamp);
    }
  }
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

  panel->SetInput(input);
  panel->SetTimebase(view->timebase());
  panel->SetTimestamp(view->GetTimestamp());
  panel->SetTimeTarget(view->GetTimeTarget());

  connect(view, &NodeParamView::TimebaseChanged, panel, &CurvePanel::SetTimebase);
  connect(view, &NodeParamView::TimeChanged, panel, &CurvePanel::SetTimestamp);
  connect(view, &NodeParamView::TimeTargetChanged, panel, &CurvePanel::SetTimeTarget);
  connect(panel, &CurvePanel::TimeChanged, view, &NodeParamView::SetTimestamp);
  connect(panel, &CurvePanel::TimeChanged, view, &NodeParamView::TimeChanged);
  connect(panel, &CurvePanel::CloseRequested, this, &ParamPanel::ClosingCurvePanel);

  open_curve_panels_.insert(input, panel);
}

void ParamPanel::OpeningNode(Node *n)
{
  QList<NodeInput*> inputs = n->GetInputsIncludingArrays();

  foreach (NodeInput* i, inputs) {
    if (open_curve_panels_.contains(i)) {
      // We had a CurvePanel open for this input that was closed in ClosingNode(), re-open it
      CreateCurvePanel(i);
    }
  }
}

void ParamPanel::ClosingNode(Node *n)
{
  QList<NodeInput*> inputs = n->GetInputsIncludingArrays();

  foreach (NodeInput* i, inputs) {
    CurvePanel* panel = open_curve_panels_.value(i);

    // Close the panel (this also destroys it), but keep a reference in the hash
    if (panel) {
      panel->close();
      open_curve_panels_.insert(i, nullptr);
    }
  }
}

void ParamPanel::ClosingCurvePanel()
{
  CurvePanel* panel = static_cast<CurvePanel*>(sender());
  open_curve_panels_.remove(panel->GetInput());
}

OLIVE_NAMESPACE_EXIT
