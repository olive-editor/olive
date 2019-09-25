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

ToolPanel::ToolPanel(QWidget *parent) :
  PanelWidget(parent)
{
  // FIXME: This won't work if there's ever more than one of this panel
  setObjectName("ToolPanel");

  Toolbar* t = new Toolbar(this);

  t->SetTool(olive::core.tool());
  t->SetSnapping(olive::core.snapping());

  setWidget(t);

  connect(t, SIGNAL(ToolChanged(const olive::tool::Tool&)), &olive::core, SLOT(SetTool(const olive::tool::Tool&)));
  connect(&olive::core, SIGNAL(ToolChanged(const olive::tool::Tool&)), t, SLOT(SetTool(const olive::tool::Tool&)));

  connect(t, SIGNAL(SnappingChanged(const bool&)), &olive::core, SLOT(SetSnapping(const bool&)));
  connect(&olive::core, SIGNAL(SnappingChanged(const bool&)), t, SLOT(SetSnapping(const bool&)));

  Retranslate();
}

void ToolPanel::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
  QDockWidget::changeEvent(e);
}

void ToolPanel::Retranslate()
{
  SetTitle(tr("Tools"));
}
