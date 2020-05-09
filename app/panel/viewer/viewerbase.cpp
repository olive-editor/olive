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

#include "viewerbase.h"

#include "window/mainwindow/mainwindow.h"

OLIVE_NAMESPACE_ENTER

ViewerPanelBase::ViewerPanelBase(const QString& object_name, QWidget *parent) :
  TimeBasedPanel(object_name, parent)
{
}

void ViewerPanelBase::PlayPause()
{
  static_cast<ViewerWidget*>(GetTimeBasedWidget())->TogglePlayPause();
}

void ViewerPanelBase::PlayInToOut()
{
  static_cast<ViewerWidget*>(GetTimeBasedWidget())->Play(true);
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
  connect(panel, &TimeBasedPanel::PlayInToOutRequested, this, &ViewerPanelBase::PlayInToOut);
  connect(panel, &TimeBasedPanel::ShuttleLeftRequested, this, &ViewerPanelBase::ShuttleLeft);
  connect(panel, &TimeBasedPanel::ShuttleStopRequested, this, &ViewerPanelBase::ShuttleStop);
  connect(panel, &TimeBasedPanel::ShuttleRightRequested, this, &ViewerPanelBase::ShuttleRight);
}

void ViewerPanelBase::DisconnectTimeBasedPanel(TimeBasedPanel *panel)
{
  disconnect(panel, &TimeBasedPanel::PlayPauseRequested, this, &ViewerPanelBase::PlayPause);
  disconnect(panel, &TimeBasedPanel::PlayInToOutRequested, this, &ViewerPanelBase::PlayInToOut);
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

void ViewerPanelBase::SetGizmos(Node *node)
{
  static_cast<ViewerWidget*>(GetTimeBasedWidget())->SetGizmos(node);
}

void ViewerPanelBase::CreateScopePanel(ScopePanel::Type type)
{
  ViewerWidget* vw = static_cast<ViewerWidget*>(GetTimeBasedWidget());
  ScopePanel* p = Core::instance()->main_window()->AppendScopePanel();

  p->SetType(type);

  // Connect viewer widget texture drawing to scope panel
  connect(vw, &ViewerWidget::LoadedBuffer, p, &ScopePanel::SetReferenceBuffer);
  connect(vw, &ViewerWidget::ColorManagerChanged, p, &ScopePanel::SetColorManager);

  p->SetColorManager(vw->color_manager());

  vw->ForceUpdate();
}

void ViewerPanelBase::showEvent(QShowEvent *e)
{
  static_cast<ViewerWidget*>(GetTimeBasedWidget())->Pause();

  TimeBasedPanel::showEvent(e);
}

void ViewerPanelBase::closeEvent(QCloseEvent *e)
{
  static_cast<ViewerWidget*>(GetTimeBasedWidget())->Pause();

  TimeBasedPanel::closeEvent(e);
}

OLIVE_NAMESPACE_EXIT
