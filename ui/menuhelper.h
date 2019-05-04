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

#ifndef MENUHELPER_H
#define MENUHELPER_H

#include <QObject>
#include <QMenuBar>

#include "ui/menu.h"

class MenuHelper : public QObject {
  Q_OBJECT
public:
  void InitializeSharedMenus();

  /**
   * @brief Creates a menu of new items that can be created
   *
   * Adds the full set of creatable items to a QMenu (e.g. new project,
   * new sequence, new folder, etc.)
   *
   * @param parent
   *
   * The menu to add items to.
   */
  void make_new_menu(QMenu* parent);

  /**
   * @brief Creates a menu of options for working with in/out points
   *
   * Adds a set of options for working with sequence/footage in/out points,
   * e.g. setting in/out points, clearing in/out points, etc.
   *
   * @param parent
   *
   * The menu to add items to.
   */
  void make_inout_menu(QMenu* parent);

  /**
   * @brief Creates a menu of clip functions
   *
   * Adds a set of clip functions including:
   * * Add Default Transition
   * * Link/Unlink
   * * Enable/Disable
   * * Nest
   *
   * @param parent
   *
   * The menu to add items to.
   */
  void make_clip_functions_menu(QMenu* parent);

  /**
   * @brief Creates standard edit menu (cut, copy, paste, etc.)
   *
   * @param parent
   *
   * The menu to add items to.
   *
   * @param objects_are_selected
   *
   * Some extra functions may be hidden in the event no clip is actually selected. Set this to **FALSE** to hide those
   * functions.
   */
  void make_edit_functions_menu(QMenu* parent, bool objects_are_selected = true);

  /**
   * @brief Sets the checked state of a menu item based on a Boolean variable.
   *
   * Many menu items simply toggle a Boolean variable. This is a convenience function, assuming the QAction's data
   * variable is a pointer to a Boolean variable, that sets the checked state of the QAction to the enabled state
   * of the Boolean. Used heavily in functions like toolMenu_About_To_Be_Shown()
   *
   * @param a
   *
   * The QAction to set the checked state of.
   */
  void set_bool_action_checked(QAction* a);

  /**
   * @brief Sets the checked state of a menu item based on an integer variable.
   *
   * Many menu items simply set a variable to a particular integer. This is a convenience function, assuming the
   * QAction's data variable is an integer to set a variable to, that sets the checked state of the QAction to
   * whether the QAction's integer equals the integer variable. Used heavily in functions like
   * viewMenu_About_To_Be_Shown()
   *
   * @param a
   *
   * The QAction to set the checked state of
   *
   * @param i
   *
   * The integer variable to compare the QAction's integer to
   */
  void set_int_action_checked(QAction* a, const int& i);

  /**
   * @brief Sets the checked state of a menu item based on a QPushButton.
   *
   * Some menu items function largely as a proxy to a QPushButton. Assuming the QAction's data variable is a
   * pointer to a QPushButton, this sets a QAction's checked state to the checked state of the QPushButton.
   *
   * @param a
   */
  void set_button_action_checked(QAction* a);

  void Retranslate();

  static Menu *create_submenu(QMenuBar* parent,
                               const QObject *receiver = nullptr,
                               const char *member = nullptr);
  static Menu* create_submenu(QMenu* parent);
  static QAction* create_menu_action(QWidget *parent,
                                     const char* id,
                                     const QObject *receiver = nullptr,
                                     const char *member = nullptr,
                                     const QKeySequence &shortcut = 0);

public slots:

  /**
   * @brief Sets a QAction's Boolean reference to the opposite of its current value
   *
   * Many menu items simply toggle a Boolean variable. This is a convenience function, assuming the QAction's data
   * variable is a pointer to a Boolean variable, that sets the Boolean variable to the opposite of its current value.
   */
  void toggle_bool_action();

  /**
   * @brief Set Title/Action Safe Area from QAction
   *
   * A receiver for several Title/Action Safe Area setting items. Assumes the sender() is a QAction with a data
   * variable as a `double`. The `double` can be the following values:
   * * NaN (qSNaN()) - Disable Title/Action Safe Area
   * * 0 - Enable Title/Action Safe Area, default aspect ratio (match current active Sequence's aspect ratio).
   * * Negative Value - Enable Title/Action Safe Area, any negative number assumes a custom aspect ratio. Will ask
   * the user to enter an aspect ratio and will use the result.
   * * Positive Value - Enable Title/Action Safe Area, use value as the aspect ratio.
   */
  void set_titlesafe_from_menu();

  /**
   * @brief Set Autoscroll setting from QAction
   *
   * Assumes the sender() is a QAction with an integer as its data variable. The data variable should be
   * `AUTOSCROLL_NO_SCROLL`, `AUTOSCROLL_PAGE_SCROLL` (default) or `AUTOSCROLL_SMOOTH_SCROLL`.
   */
  void set_autoscroll();

  /**
   * @brief Clicks a QPushButton referenced by a QAction when triggered.
   *
   * Some menu items function largely as a proxy to a QPushButton. Assuming the QAction's data variable is a
   * pointer to a QPushButton, this triggers a click() event on that QPushButton.
   */
  void menu_click_button();

  /**
   * @brief Sets the current timecode setting
   *
   * Assumes the sender() is a QAction with an integer as its data variable. The data variable should be
   * `AUTOSCROLL_NO_AUTOSCROLL`, `AUTOSCROLL_PAGE_AUTOSCROLL` (default) or `AUTOSCROLL_SMOOTH_AUTOSCROLL`.
   */
  void set_timecode_view();

  /**
   * @brief Calls open_recent() in Olive::Global using the index from a QAction
   *
   * Assumes the sender() is a QAction with an integer as its data variable. The data variable is an index of
   * the internal auto-recovery project list.
   */
  void open_recent_from_menu();

  /**
   * @brief Create a "Paste" action on the specified menu that's enabled only if the clipboard contains effects
   *
   * @param menu
   *
   * Menu to add action to
   */
  void create_effect_paste_action(QMenu *menu);

private:
  QAction* new_project_;
  QAction* new_sequence_;
  QAction* new_folder_;

  QAction* set_in_point_;
  QAction* set_out_point_;
  QAction* reset_in_point_;
  QAction* reset_out_point_;
  QAction* clear_inout_point;

  QAction* add_default_transition_;
  QAction* link_unlink_;
  QAction* enable_disable_;
  QAction* nest_;

  QAction* cut_;
  QAction* copy_;
  QAction* paste_;
  QAction* paste_insert_;
  QAction* duplicate_;
  QAction* delete_;
  QAction* ripple_delete_;
  QAction* split_;

private slots:


};

namespace olive {
/**
     * @brief A global MenuHelper object to assist menu creation throughout Olive.
     */
extern MenuHelper MenuHelper;
};

#endif // MENUHELPER_H
