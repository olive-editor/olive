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
  ViewerPanelBase(parent)
{
  // FIXME: This won't work if there's ever more than one of this panel
  setObjectName("ViewerPanel");

  // QObject system handles deleting this
  viewer_ = new ViewerWidget(this);
  connect(viewer_, SIGNAL(TimeChanged(const int64_t&)), this, SIGNAL(TimeChanged(const int64_t&)));

  // Set ViewerWidget as the central widget
  setWidget(viewer_);

  // Set strings
  Retranslate();
}

void ViewerPanel::ConnectViewerNode(ViewerOutput *node)
{
  if (viewer_->GetConnectedViewer()) {
    disconnect(viewer_->GetConnectedViewer(), &ViewerOutput::MediaNameChanged, this, &ViewerPanel::SetSubtitle);
    Retranslate();
  }

  viewer_->ConnectViewerNode(node);

  if (node) {
    connect(node, &ViewerOutput::MediaNameChanged, this, &ViewerPanel::SetSubtitle);
    SetSubtitle(node->media_name());
  }
}

rational ViewerPanel::GetTime()
{
  return viewer_->GetTime();
}

ViewerOutput *ViewerPanel::GetConnectedViewer() const
{
  return viewer_->GetConnectedViewer();
}

void ViewerPanel::SetTime(const int64_t &timestamp)
{
  viewer_->SetTime(timestamp);
}

void ViewerPanel::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
  PanelWidget::changeEvent(e);
}

void ViewerPanel::Retranslate()
{
  SetTitle(tr("Viewer"));

  if (!viewer_->GetConnectedViewer()) {
    SetSubtitle(tr("(none)"));
  }
}
