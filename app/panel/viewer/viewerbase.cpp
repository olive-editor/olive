#include "viewerbase.h"

ViewerPanelBase::ViewerPanelBase(QWidget *parent) :
  PanelWidget(parent)
{
}

void ViewerPanelBase::ZoomIn()
{
  viewer_->SetScale(viewer_->scale() * 2);
}

void ViewerPanelBase::ZoomOut()
{
  viewer_->SetScale(viewer_->scale() * 0.5);
}

void ViewerPanelBase::GoToStart()
{
  viewer_->GoToStart();
}

void ViewerPanelBase::PrevFrame()
{
  viewer_->PrevFrame();
}

void ViewerPanelBase::PlayPause()
{
  viewer_->TogglePlayPause();
}

void ViewerPanelBase::NextFrame()
{
  viewer_->NextFrame();
}

void ViewerPanelBase::GoToEnd()
{
  viewer_->GoToEnd();
}

void ViewerPanelBase::ShuttleLeft()
{
  viewer_->ShuttleLeft();
}

void ViewerPanelBase::ShuttleStop()
{
  viewer_->ShuttleStop();
}

void ViewerPanelBase::ShuttleRight()
{
  viewer_->ShuttleRight();
}
