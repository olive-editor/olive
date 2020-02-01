#include "curve.h"

CurvePanel::CurvePanel(QWidget *parent) :
  TimeBasedPanel(parent)
{
  // FIXME: This won't work if there's ever more than one of this panel
  setObjectName("CurvePanel");

  // Create main widget and set it
  SetTimeBasedWidget(new CurveWidget());

  // Set strings
  Retranslate();
}

void CurvePanel::SetInput(NodeInput *input)
{
  static_cast<CurveWidget*>(GetTimeBasedWidget())->SetInput(input);
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
}
