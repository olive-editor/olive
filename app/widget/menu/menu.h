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

#ifndef WIDGETMENU_H
#define WIDGETMENU_H

#include <QMenuBar>
#include <QMenu>
#include <QMetaMethod>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A menu widget for context menus and menu bars
 *
 * A QMenu subclass with functions for creating menus and menu items that conform to Olive's menu and keyboard shortcut
 * system.
 *
 * In Olive, menu items in the menu bar are also responsible for keyboard shortcuts throughout the application. To allow
 * these to be configurable and these configurations saveable, every item needs a unique ID. This ID gets linked to the
 * keyboard shortcuts in config files. The ID doesn't get translated so it can also persist through language changes.
 *
 * The ID gets stored in the QAction's "id" property. If a keyboard shortcut is provided, it gets stored in the
 * QAction's "keydefault" property.
 *
 * It is always recommended to use this over QMenu in any situation.
 */
class Menu : public QMenu
{
public:
  Menu(QMenuBar* bar);

  template <typename Func>
  /**
   * @brief Construct a Menu and add it to a QMenuBar
   *
   * This Menu can be connected to a slot that's triggered when the Menu is "about to show". Use `receiver` and
   * `member` to connect this (same syntax as QObject::connect) or leave as nullptr to not.
   */
  Menu(QMenuBar* bar,
       const typename QtPrivate::FunctionPointer<Func>::Object *receiver,
       Func member)
  {
    bar->addMenu(this);

    Init();
    ConnectAboutToShow(receiver, member);
  }

  Menu(Menu* menu);

  template <typename Func>
  /**
   * @brief Construct a Menu and add it as a submenu to another Menu
   *
   * This Menu can be connected to a slot that's triggered when the Menu is "about to show". Use `receiver` and
   * `member` to connect this (same syntax as QObject::connect) or leave as nullptr to not.
   */
  Menu(Menu* menu,
       const typename QtPrivate::FunctionPointer<Func>::Object *receiver,
       Func member)
  {
    menu->addMenu(this);

    Init();
    ConnectAboutToShow(receiver, member);
  }

  /**
   * @brief Construct a popup menu
   */
  Menu(QWidget* parent = nullptr);

  /**
   * @brief Construct a popup menu
   */
  Menu(const QString& s, QWidget* parent = nullptr);

  template <typename Func>
  /**
   * @brief Create a menu item and add it to this menu
   *
   * @param id
   *
   * The action's unique ID
   *
   * @param receiver
   *
   * The QObject to receive the signal when this item is triggered
   *
   * @param member
   *
   * The QObject slot to connect this action's triggered signal to
   *
   * @param key
   *
   * Default keyboard sequence
   *
   * @return
   *
   * The QAction that was created and added to this Menu
   */
  QAction* AddItem(const QString& id,
                   const typename QtPrivate::FunctionPointer<Func>::Object *receiver,
                   Func member,
                   const QString &key = QString())
  {
    QAction* a = CreateItem(this, id, receiver, member, key);

    addAction(a);

    return a;
  }

  QAction* AddActionWithData(const QString& text,
                             const QVariant& d,
                             const QVariant& compare);

  QAction *InsertAlphabetically(const QString& s);
  void InsertAlphabetically(QAction* entry);
  void InsertAlphabetically(Menu* menu);

  template <typename Func>
  /**
   * @brief Create a menu item
   *
   * @param parent
   *
   * The QAction's parent
   *
   * @param id
   *
   * The action's unique ID
   *
   * @param receiver
   *
   * The QObject to receive the signal when this item is triggered
   *
   * @param member
   *
   * The QObject slot to connect this action's triggered signal to
   *
   * @param key
   *
   * Default keyboard sequence
   *
   * @return
   *
   * The QAction that was created and added to this Menu
   */
  static QAction* CreateItem(QObject* parent,
                             const QString& id,
                             const typename QtPrivate::FunctionPointer<Func>::Object *receiver,
                             Func member,
                             const QString& key = QString())
  {
    QAction* a = new QAction(parent);

    ConformItem(a,
                id,
                receiver,
                member,
                key);

    return a;
  }

  template <typename Func>
  /**
   * @brief Conform a QAction to Olive's ID/keydefault system
   *
   * If a QAction was created elsewhere (e.g. through QUndoStack::createUndoAction()), this function will give it
   * properties conforming it to Olive's menu item system
   *
   * @param a
   *
   * The QAction's to conform
   *
   * @param id
   *
   * The action's unique ID
   *
   * @param receiver
   *
   * The QObject to receive the signal when this item is triggered
   *
   * @param member
   *
   * The QObject slot to connect this action's triggered signal to
   *
   * @param key
   *
   * Default keyboard sequence
   */
  static void ConformItem(QAction *a,
                          const QString& id,
                          const typename QtPrivate::FunctionPointer<Func>::Object *receiver,
                          Func member,
                          const QString& key = QString())
  {
    ConformItem(a, id, key);

    connect(a, &QAction::triggered, receiver, member);
  }

  static void ConformItem(QAction *a,
                          const QString& id,
                          const QString& key = QString());

  static void SetBooleanAction(QAction* a, bool *boolean);

private:
  void Init();

  template <typename Func>
  void ConnectAboutToShow(const typename QtPrivate::FunctionPointer<Func>::Object *receiver, Func member)
  {
    connect(this, &Menu::aboutToShow, receiver, member);
  }

};

OLIVE_NAMESPACE_EXIT

#endif // WIDGETMENU_H
