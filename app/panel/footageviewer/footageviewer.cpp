/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

namespace olive {

FootageViewerPanel::FootageViewerPanel(QWidget *parent) :
  ViewerPanelBase(QStringLiteral("FootageViewerPanel"), parent)
{
  // Set ViewerWidget as the central widget
  FootageViewerWidget* fvw = new FootageViewerWidget();
  fvw->SetAutoCacheEnabled(false);
  connect(fvw, &FootageViewerWidget::RequestScopePanel, this, &FootageViewerPanel::CreateScopePanel);
  SetTimeBasedWidget(fvw);

  // Set strings
  Retranslate();
}

QList<Footage *> FootageViewerPanel::GetSelectedFootage() const
{
  QList<Footage *> list;
  Footage* f = static_cast<FootageViewerWidget*>(GetTimeBasedWidget())->GetFootage();

  if (f) {
    list.append(f);
  }

  return list;
}

void FootageViewerPanel::SetFootage(Footage *f)
{
  if (f && !f->IsValid()) {
    // Do nothing if footage is invalid
    return;
  }

  static_cast<FootageViewerWidget*>(GetTimeBasedWidget())->SetFootage(f);

  if (f) {
    SetSubtitle(f->name());
  } else {
    Retranslate();
  }
}

void FootageViewerPanel::Retranslate()
{
  ViewerPanelBase::Retranslate();

  SetTitle(tr("Footage Viewer"));
}

}
