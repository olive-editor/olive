/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

namespace olive {

ViewerPanel::ViewerPanel(const QString &object_name, QWidget *parent) :
  ViewerPanelBase(object_name, parent)
{
  Init();
}

ViewerPanel::ViewerPanel(QWidget *parent) :
  ViewerPanelBase(QStringLiteral("ViewerPanel"), parent)
{
  Init();
}

void ViewerPanel::Retranslate()
{
  ViewerPanelBase::Retranslate();

  SetTitle(tr("Viewer"));
}

void ViewerPanel::Init()
{
  // Set ViewerWidget as the central widget
  ViewerWidget* vw = new ViewerWidget();
  connect(vw, &ViewerWidget::RequestScopePanel, this, &ViewerPanel::CreateScopePanel);
  SetTimeBasedWidget(vw);

  // Set strings
  Retranslate();
}

}
