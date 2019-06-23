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

#include "menu.h"

#include "ui/style/style.h"

Menu::Menu(QMenuBar *bar, const QObject* receiver, const char* member) :
  QMenu(bar)
{
  SetStyling();

  bar->addMenu(this);

  if (receiver != nullptr) {
    connect(this, SIGNAL(aboutToShow()), receiver, member);
  }
}

Menu::Menu(Menu *menu, const QObject *receiver, const char *member)
{
  SetStyling();

  menu->addMenu(this);

  if (receiver != nullptr) {
    connect(this, SIGNAL(aboutToShow()), receiver, member);
  }
}

QAction *Menu::AddItem(const QString &id,
                       const QObject *receiver,
                       const char *member,
                       const QString &key)
{
  QAction* a = CreateItem(this, id, receiver, member, key);

  addAction(a);

  return a;
}

QAction *Menu::CreateItem(QObject* parent,
                          const QString &id,
                          const QObject *receiver,
                          const char *member,
                          const QString &key)
{
  QAction* a = new QAction(parent);

  a->setProperty("id", id);

  if (!key.isEmpty()) {
    a->setShortcut(key);
    a->setProperty("keydefault", key);
  }

  connect(a, SIGNAL(triggered(bool)), receiver, member);

  return a;
}

void Menu::SetBooleanAction(QAction *a, bool* boolean)
{
  // FIXME: Connect to some boolean function
  a->setCheckable(true);
  a->setChecked(*boolean);
  a->setProperty("boolptr", reinterpret_cast<quintptr>(boolean));
}

void Menu::SetStyling()
{
  //if (olive::config.use_native_menu_styling) {
    olive::style::WidgetSetNative(this);
  //}
}
