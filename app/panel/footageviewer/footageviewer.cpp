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

#include "footageviewer.h"

#include "widget/viewer/footageviewer.h"

FootageViewerPanel::FootageViewerPanel(QWidget *parent) :
  ViewerPanelBase(parent)
{
  // FIXME: This won't work if there's ever more than one of this panel
  setObjectName("FootageViewerPanel");

  // QObject system handles deleting this
  viewer_ = new FootageViewerWidget(this);

  // Set ViewerWidget as the central widget
  setWidget(viewer_);

  // Set strings
  Retranslate();
}

void FootageViewerPanel::SetFootage(Footage *f)
{
  static_cast<FootageViewerWidget*>(viewer_)->SetFootage(f);
}

void FootageViewerPanel::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
  PanelWidget::changeEvent(e);
}

void FootageViewerPanel::Retranslate()
{
  SetTitle(tr("Footage Viewer"));
  SetSubtitle(tr("(none)"));
}
