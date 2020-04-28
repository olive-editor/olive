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

#include "curve.h"

OLIVE_NAMESPACE_ENTER

CurvePanel::CurvePanel(QWidget *parent) :
  TimeBasedPanel(QStringLiteral("CurvePanel"), parent)
{
  // Create main widget and set it
  SetTimeBasedWidget(new CurveWidget());

  // Set strings
  Retranslate();
}

NodeInput *CurvePanel::GetInput() const
{
  return static_cast<CurveWidget*>(GetTimeBasedWidget())->GetInput();
}

void CurvePanel::SetInput(NodeInput *input)
{
  static_cast<CurveWidget*>(GetTimeBasedWidget())->SetInput(input);

  Retranslate();
}

void CurvePanel::SetTimeTarget(Node *target)
{
  static_cast<CurveWidget*>(GetTimeBasedWidget())->SetTimeTarget(target);
}

void CurvePanel::IncreaseTrackHeight()
{
  CurveWidget* c = static_cast<CurveWidget*>(GetTimeBasedWidget());
  c->SetVerticalScale(c->GetVerticalScale() * 2);
}

void CurvePanel::DecreaseTrackHeight()
{
  CurveWidget* c = static_cast<CurveWidget*>(GetTimeBasedWidget());
  c->SetVerticalScale(c->GetVerticalScale() * 0.5);
}

void CurvePanel::Retranslate()
{
  TimeBasedPanel::Retranslate();

  SetTitle(tr("Curve Editor"));

  NodeInput* connected_input = static_cast<CurveWidget*>(GetTimeBasedWidget())->GetInput();
  if (connected_input) {
    SetSubtitle(connected_input->name());
  } else {
    SetSubtitle(QString());
  }
}

OLIVE_NAMESPACE_EXIT
