#include "timebased.h"

TimeBasedPanel::TimeBasedPanel(QWidget *parent) :
  PanelWidget(parent),
  widget_(nullptr)
{
}

rational TimeBasedPanel::GetTime()
{
  return widget_->GetTime();
}

void TimeBasedPanel::GoToStart()
{
  widget_->GoToStart();
}

void TimeBasedPanel::PrevFrame()
{
  widget_->PrevFrame();
}

void TimeBasedPanel::NextFrame()
{
  widget_->NextFrame();
}

void TimeBasedPanel::GoToEnd()
{
  widget_->GoToEnd();
}

void TimeBasedPanel::ZoomIn()
{
  widget_->ZoomIn();
}

void TimeBasedPanel::ZoomOut()
{
  widget_->ZoomOut();
}

void TimeBasedPanel::SetTimebase(const rational &timebase)
{
  widget_->SetTimebase(timebase);
}

void TimeBasedPanel::SetTime(const int64_t &timestamp)
{
  widget_->SetTime(timestamp);
}

void TimeBasedPanel::GoToPrevCut()
{
  widget_->GoToPrevCut();
}

void TimeBasedPanel::GoToNextCut()
{
  widget_->GoToNextCut();
}

void TimeBasedPanel::PlayPause()
{
  emit PlayPauseRequested();
}

void TimeBasedPanel::ShuttleLeft()
{
  emit ShuttleLeftRequested();
}

void TimeBasedPanel::ShuttleStop()
{
  emit ShuttleStopRequested();
}

void TimeBasedPanel::ShuttleRight()
{
  emit ShuttleRightRequested();
}

TimeBasedWidget *TimeBasedPanel::GetTimeBasedWidget() const
{
  return widget_;
}

ViewerOutput *TimeBasedPanel::GetConnectedViewer() const
{
  return widget_->GetConnectedNode();
}

TimeRuler *TimeBasedPanel::ruler() const
{
  return widget_->ruler();
}

void TimeBasedPanel::ConnectViewerNode(ViewerOutput *node)
{
  if (widget_->GetConnectedNode()) {
    disconnect(widget_->GetConnectedNode(), &ViewerOutput::MediaNameChanged, this, &TimeBasedPanel::SetSubtitle);
    Retranslate();
  }

  widget_->ConnectViewerNode(node);

  if (node) {
    connect(node, &ViewerOutput::MediaNameChanged, this, &TimeBasedPanel::SetSubtitle);
    SetSubtitle(node->media_name());
  }
}

void TimeBasedPanel::SetTimeBasedWidget(TimeBasedWidget *widget)
{
  if (widget_) {
    disconnect(widget_, &TimeBasedWidget::TimeChanged, this, &TimeBasedPanel::TimeChanged);
    disconnect(widget_, &TimeBasedWidget::TimebaseChanged, this, &TimeBasedPanel::TimebaseChanged);
  }

  widget_ = widget;

  if (widget_) {
    connect(widget_, &TimeBasedWidget::TimeChanged, this, &TimeBasedPanel::TimeChanged);
    connect(widget_, &TimeBasedWidget::TimebaseChanged, this, &TimeBasedPanel::TimebaseChanged);
  }

  setWidget(widget_);
}

void TimeBasedPanel::Retranslate()
{
  if (!GetTimeBasedWidget()->GetConnectedNode()) {
    SetSubtitle(tr("(none)"));
  }
}

void TimeBasedPanel::SetIn()
{
  GetTimeBasedWidget()->SetInAtPlayhead();
}

void TimeBasedPanel::SetOut()
{
  GetTimeBasedWidget()->SetOutAtPlayhead();
}

void TimeBasedPanel::ResetIn()
{
  GetTimeBasedWidget()->ResetIn();
}

void TimeBasedPanel::ResetOut()
{
  GetTimeBasedWidget()->ResetOut();
}

void TimeBasedPanel::ClearInOut()
{
  GetTimeBasedWidget()->ClearInOutPoints();
}

void TimeBasedPanel::SetMarker()
{
  GetTimeBasedWidget()->SetMarker();
}
