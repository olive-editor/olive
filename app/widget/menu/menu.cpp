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

#include "menu.h"

#include "ui/style/style.h"

namespace olive {

Menu::Menu(QMenuBar *bar)
{
  bar->addMenu(this);

  Init();
}

Menu::Menu(Menu *menu)
{
  menu->addMenu(this);

  Init();
}

Menu::Menu(QWidget *parent) :
  QMenu(parent)
{
  Init();
}

Menu::Menu(const QString &s, QWidget *parent) :
  QMenu(s, parent)
{
  Init();
}

QAction *Menu::AddActionWithData(const QString &text, const QVariant &d, const QVariant &compare)
{
  QAction* a = addAction(text);

  a->setData(d);
  a->setCheckable(true);
  a->setChecked(d == compare);

  return a;
}

QAction* Menu::InsertAlphabetically(const QString &s)
{
  QAction* action = new QAction(s, this);
  InsertAlphabetically(action);
  return action;
}

void Menu::InsertAlphabetically(QAction *entry)
{
  QList<QAction*> actions = this->actions();

  foreach (QAction* action, actions) {
    if (action->text() > entry->text()) {
      insertAction(action, entry);
      return;
    }
  }

  addAction(entry);
}

void Menu::InsertAlphabetically(Menu *menu)
{
  InsertAlphabetically(menu->menuAction());
}

void Menu::ConformItem(QAction *a, const QString &id, const QString &key)
{
  a->setProperty("id", id);

  if (!key.isEmpty()) {
    a->setShortcut(key);
    a->setProperty("keydefault", key);

    // Set to application context so that ViewerWindows still trigger shortcuts
    a->setShortcutContext(Qt::ApplicationShortcut);
  }
}

void Menu::Init()
{
  // HACK: Disables embossing on disabled text for a slightly nicer UI
  QPalette p = palette();
  p.setColor(QPalette::Disabled, QPalette::Light, QColor(0, 0, 0, 0));
  setPalette(p);

  // If a native palette is available, it should override the one above
  StyleManager::UseOSNativeStyling(this);
}

}
