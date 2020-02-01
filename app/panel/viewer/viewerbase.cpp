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
