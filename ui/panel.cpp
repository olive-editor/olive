/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "panel.h"

#include <QEvent>
#include <QDebug>

QVector<Panel*> olive::panels;

Panel::Panel(QWidget *parent) : QDockWidget (parent) {
  olive::panels.append(this);
}

Panel::~Panel()
{
  olive::panels.removeAll(this);
}

bool Panel::focused()
{
  return hasFocus();
}

void Panel::LoadLayoutState(const QByteArray &) {}

QByteArray Panel::SaveLayoutState()
{
  return QByteArray();
}

void Panel::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    /**

      NOTE: While overriding changeEvent() is the official documented way of handling runtime language change events,
      I found it buggy to do it this way (some panels would change and others wouldn't, and the panels that did/didn't
      change would be different each time). The current workaround is calling Retranslate() on each panel manually
      from MainWindow::Retranslate which is triggered by its own changeEvent() that seems fairly reliable. Currently
      this function is mostly a no-op.

      */
//    Retranslate();
  } else {
    QDockWidget::changeEvent(e);
  }
}
