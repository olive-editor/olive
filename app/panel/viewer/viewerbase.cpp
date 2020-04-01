#include "viewerbase.h"

ViewerPanelBase::ViewerPanelBase(QWidget *parent) :
  TimeBasedPanel(parent)
{
}

void ViewerPanelBase::PlayPause()
{
  static_cast<ViewerWidget*>(GetTimeBasedWidget())->TogglePlayPause();
}

void ViewerPanelBase::ShuttleLeft()
{
  static_cast<ViewerWidget*>(GetTimeBasedWidget())->ShuttleLeft();
}

void ViewerPanelBase::ShuttleStop()
{
  static_cast<ViewerWidget*>(GetTimeBasedWidget())->ShuttleStop();
}

void ViewerPanelBase::ShuttleRight()
{
  static_cast<ViewerWidget*>(GetTimeBasedWidget())->ShuttleRight();
}

void ViewerPanelBase::ConnectTimeBasedPanel(TimeBasedPanel *panel)
{
  connect(panel, &TimeBasedPanel::PlayPauseRequested, this, &ViewerPanelBase::PlayPause);
  connect(panel, &TimeBasedPanel::ShuttleLeftRequested, this, &ViewerPanelBase::ShuttleLeft);
  connect(panel, &TimeBasedPanel::ShuttleStopRequested, this, &ViewerPanelBase::ShuttleStop);
  connect(panel, &TimeBasedPanel::ShuttleRightRequested, this, &ViewerPanelBase::ShuttleRight);
}

void ViewerPanelBase::DisconnectTimeBasedPanel(TimeBasedPanel *panel)
{
  disconnect(panel, &TimeBasedPanel::PlayPauseRequested, this, &ViewerPanelBase::PlayPause);
  disconnect(panel, &TimeBasedPanel::ShuttleLeftRequested, this, &ViewerPanelBase::ShuttleLeft);
  disconnect(panel, &TimeBasedPanel::ShuttleStopRequested, this, &ViewerPanelBase::ShuttleStop);
  disconnect(panel, &TimeBasedPanel::ShuttleRightRequested, this, &ViewerPanelBase::ShuttleRight);
}

VideoRenderBackend *ViewerPanelBase::video_renderer() const
{
  return static_cast<ViewerWidget*>(GetTimeBasedWidget())->video_renderer();
}

void ViewerPanelBase::ConnectPixelSamplerPanel(PixelSamplerPanel *psp)
{
  ViewerWidget* vw = static_cast<ViewerWidget*>(GetTimeBasedWidget());

  connect(psp, &PixelSamplerPanel::visibilityChanged, vw, &ViewerWidget::SetSignalCursorColorEnabled);
  connect(vw, &ViewerWidget::CursorColor, psp, &PixelSamplerPanel::SetValues);
}

void ViewerPanelBase::SetFullScreen(QScreen *screen)
{
  static_cast<ViewerWidget*>(GetTimeBasedWidget())->SetFullScreen(screen);
}
