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

  // Set ViewerWidget as the central widget
  SetTimeBasedWidget(new ViewerWidget());

  // Set strings
  Retranslate();
}

void ViewerPanel::Retranslate()
{
  ViewerPanelBase::Retranslate();

  SetTitle(tr("Viewer"));
}
