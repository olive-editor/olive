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

OLIVE_NAMESPACE_ENTER

Menu::Menu(QMenuBar *bar, const QObject* receiver, const char* member) :
  QMenu(bar)
{
  StyleManager::UseNativeWindowsStyling(this);

  bar->addMenu(this);

  if (receiver != nullptr) {
    connect(this, SIGNAL(aboutToShow()), receiver, member);
  }
}

Menu::Menu(Menu *menu, const QObject *receiver, const char *member) :
  QMenu(menu)
{
  StyleManager::UseNativeWindowsStyling(this);

  menu->addMenu(this);

  if (receiver != nullptr) {
    connect(this, SIGNAL(aboutToShow()), receiver, member);
  }
}

Menu::Menu(QWidget *parent) :
  QMenu(parent)
{
  StyleManager::UseNativeWindowsStyling(this);
}

Menu::Menu(const QString &s, QWidget *parent) :
  QMenu(s, parent)
{
  StyleManager::UseNativeWindowsStyling(this);
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

QAction* Menu::InsertAlphabetically(const QString &s)
{
  QAction* action = new QAction(s);
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
  QAction* action = new QAction(menu->title());
  action->setMenu(menu);
  InsertAlphabetically(action);
}

QAction *Menu::CreateItem(QObject* parent,
                          const QString &id,
                          const QObject *receiver,
                          const char *member,
                          const QString &key)
{
  QAction* a = new QAction(parent);

  ConformItem(a,
              id,
              receiver,
              member,
              key);

  return a;
}

void Menu::ConformItem(QAction* a, const QString &id, const QObject *receiver, const char *member, const QString &key)
{
  a->setProperty("id", id);

  if (!key.isEmpty()) {
    a->setShortcut(key);
    a->setProperty("keydefault", key);

    // Set to application context so that ViewerWindows still trigger shortcuts
    a->setShortcutContext(Qt::ApplicationShortcut);
  }

  if (receiver != nullptr) {
    connect(a, SIGNAL(triggered(bool)), receiver, member);
  }
}

void Menu::SetBooleanAction(QAction *a, bool* boolean)
{
  // FIXME: Connect to some boolean function
  a->setCheckable(true);
  a->setChecked(*boolean);
  a->setProperty("boolptr", reinterpret_cast<quintptr>(boolean));
}

OLIVE_NAMESPACE_EXIT
