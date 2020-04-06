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

#include "tool.h"

#include "core.h"
#include "widget/toolbar/toolbar.h"

OLIVE_NAMESPACE_ENTER

ToolPanel::ToolPanel(QWidget *parent) :
  PanelWidget(parent)
{
  // FIXME: This won't work if there's ever more than one of this panel
  setObjectName("ToolPanel");

  Toolbar* t = new Toolbar(this);

  t->SetTool(Core::instance()->tool());
  t->SetSnapping(Core::instance()->snapping());

  setWidget(t);

  connect(t, &Toolbar::ToolChanged, Core::instance(), &Core::SetTool);
  connect(Core::instance(), &Core::ToolChanged, t, &Toolbar::SetTool);

  connect(t, &Toolbar::SnappingChanged, Core::instance(), &Core::SetSnapping);
  connect(Core::instance(), &Core::SnappingChanged, t, &Toolbar::SetSnapping);

  connect(t, &Toolbar::AddableObjectChanged, Core::instance(), &Core::SetSelectedAddableObject);

  Retranslate();
}

void ToolPanel::Retranslate()
{
  SetTitle(tr("Tools"));
}

OLIVE_NAMESPACE_EXIT
