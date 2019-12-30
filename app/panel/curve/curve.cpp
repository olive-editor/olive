#include "curve.h"

CurvePanel::CurvePanel(QWidget *parent) :
  PanelWidget(parent)
{
  // FIXME: This won't work if there's ever more than one of this panel
  setObjectName("CurvePanel");

  // Create main widget and set it
  widget_ = new CurveWidget();
  setWidget(widget_);

  connect(widget_, &CurveWidget::TimeChanged, this, &CurvePanel::TimeChanged);

  // Set strings
  Retranslate();
}

void CurvePanel::SetInput(NodeInput *input)
{
  widget_->SetInput(input);
}

void CurvePanel::SetTimebase(const rational &timebase)
{
  widget_->SetTimebase(timebase);
}

void CurvePanel::SetTime(const int64_t &timestamp)
{
  widget_->SetTime(timestamp);
}

void CurvePanel::ZoomIn()
{
  widget_->SetScale(widget_->GetScale() * 2);
}

void CurvePanel::ZoomOut()
{
  widget_->SetScale(widget_->GetScale() * 0.5);
}

void CurvePanel::IncreaseTrackHeight()
{
  widget_->SetVerticalScale(widget_->GetVerticalScale() * 2);
}

void CurvePanel::DecreaseTrackHeight()
{
  widget_->SetVerticalScale(widget_->GetVerticalScale() * 0.5);
}

void CurvePanel::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
  PanelWidget::changeEvent(e);
}

void CurvePanel::Retranslate()
{
  SetTitle(tr("Curve Editor"));
}
