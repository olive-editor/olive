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

#ifndef ACTIONSEARCH_H
#define ACTIONSEARCH_H

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class ActionSearchList;

/**
 * @brief The ActionSearch class
 *
 * A popup window (accessible through Help > Action Search) that allows users to search for a menu command by typing
 * rather than browsing through the menu bar. This can be created from anywhere provided olive::MainWindow is valid.
 */
class ActionSearch : public QDialog
{
  Q_OBJECT
public:
  /**
   * @brief ActionSearch Constructor
   *
   * Create ActionSearch popup.
   *
   * @param parent
   *
   * QWidget parent. Usually MainWindow.
   */
  ActionSearch(QWidget* parent);

  /**
   * @brief Set the menu bar to use in this action search
   */
  void SetMenuBar(QMenuBar* menu_bar);
private slots:
  /**
   * @brief Update the list of actions according to a search query
   *
   * This function adds/removes actions in the action list according to a given search query entered by the user.
   *
   * To loop over the menubar and all of its menus and submenus, this function will call itself recursively. As such
   * some of its parameters do not need to be set externally, as these will be set by the function itself as it calls
   * itself.
   *
   * @param s
   *
   * The search text. This is the only parameter that should be set externally.
   *
   * @param p
   *
   * The current parent hierarchy. In most cases, this should be left as nullptr when called externally.
   * search_update() will fill this automatically as it needs while calling itself recursively.
   *
   * @param parent
   *
   * The current menu to loop over. In most cases, this should be left as nullptr when called externally.
   * search_update() will fill this automatically as it needs while calling itself recursively.
   */
  void search_update(const QString& s, const QString &p = nullptr, QMenu *parent = nullptr);

  /**
   * @brief Perform the currently selected action
   *
   * Usually triggered by pressing Enter on the ActionSearchEntry field, this will trigger whatever action is currently
   * highlighted and then close this popup. If no entries are highlighted (i.e. the list is empty), no action is
   * triggered and the popup closes anyway.
   */
  void perform_action();

  /**
   * @brief Move selection up
   *
   * A slot for pressing up on the ActionSearchEntry field. Moves the selection in the list up once. If the
   * selection is already at the top of the list, this is a no-op.
   */
  void move_selection_up();

  /**
   * @brief Move selection down
   *
   * A slot for pressing down on the ActionSearchEntry field. Moves the selection in the list down once. If the
   * selection is already at the bottom of the list, this is a no-op.
   */
  void move_selection_down();
private:
  /**
   * @brief Main widget that shows the list of commands
   */
  ActionSearchList* list_widget;

  /**
   * @brief Attached menu bar object
   */
  QMenuBar* menu_bar_;
};

/**
 * @brief The ActionSearchList class
 *
 * Simple wrapper around QListWidget that emits a signal when an item is double clicked that ActionSearch connects
 * to a slot that triggers the currently selected action.
 */
class ActionSearchList : public QListWidget {
  Q_OBJECT
public:
  /**
   * @brief ActionSearchList Constructor
   * @param parent
   *
   * Usually ActionSearch.
   */
  ActionSearchList(QWidget* parent);
protected:
  /**
   * @brief Override of QListWidget's double click event that emits a signal.
   */
  void mouseDoubleClickEvent(QMouseEvent *);
signals:
  /**
   * @brief Signal emitted when a QListWidget item is double clicked.
   */
  void dbl_click();
};

/**
 * @brief The ActionSearchEntry class
 *
 * Simple wrapper around QLineEdit that emits signals when the up or down arrow keys are pressed so that ActionSearch
 * can connect them to moving the current selection up or down.
 */
class ActionSearchEntry : public QLineEdit {
  Q_OBJECT
public:
  /**
   * @brief ActionSearchEntry
   * @param parent
   *
   * Usually ActionSearch.
   */
  ActionSearchEntry(QWidget* parent);
protected:
  /**
   * @brief Override of QLineEdit's key press event that listens for up/down key presses.
   * @param event
   */
  void keyPressEvent(QKeyEvent * event);
signals:
  /**
   * @brief Emitted when the user presses the up arrow key.
   */
  void moveSelectionUp();

  /**
   * @brief Emitted when the user presses the down arrow key.
   */
  void moveSelectionDown();
};

OLIVE_NAMESPACE_EXIT

#endif // ACTIONSEARCH_H
