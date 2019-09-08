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

#include "viewer.h"

ViewerPanel::ViewerPanel(QWidget *parent) :
  PanelWidget(parent)
{
  // QObject system handles deleting this
  viewer_ = new ViewerWidget(this);
  connect(viewer_, SIGNAL(TimeChanged(const rational&)), this, SIGNAL(TimeChanged(const rational&)));

  // Set ViewerWidget as the central widget
  setWidget(viewer_);

  // Set strings
  Retranslate();
}

void ViewerPanel::ZoomIn()
{
  viewer_->SetScale(viewer_->scale() * 2);
}

void ViewerPanel::ZoomOut()
{
  viewer_->SetScale(viewer_->scale() * 0.5);
}

void ViewerPanel::GoToStart()
{
  viewer_->GoToStart();
}

void ViewerPanel::PrevFrame()
{
  viewer_->PrevFrame();
}

void ViewerPanel::PlayPause()
{
  viewer_->TogglePlayPause();
}

void ViewerPanel::NextFrame()
{
  viewer_->NextFrame();
}

void ViewerPanel::GoToEnd()
{
  viewer_->GoToEnd();
}

void ViewerPanel::SetTimebase(const rational &timebase)
{
  viewer_->SetTimebase(timebase);
}

rational ViewerPanel::GetTime()
{
  return viewer_->GetTime();
}

void ViewerPanel::SetTexture(GLuint tex)
{
  viewer_->SetTexture(tex);
}

void ViewerPanel::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
  QDockWidget::changeEvent(e);
}

void ViewerPanel::Retranslate()
{
  SetTitle(tr("Viewer"));
  SetSubtitle(tr("(none)"));
}
