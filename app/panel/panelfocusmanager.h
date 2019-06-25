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

#ifndef PANELFOCUSMANAGER_H
#define PANELFOCUSMANAGER_H

#include <QObject>
#include <QList>

#include "panel/viewer/viewer.h"
#include "widget/panel/panel.h"

/**
 * @brief The PanelFocusManager class
 *
 * This object keeps track of which panel is focused at any given time.
 *
 * Sometimes a function (specifically a keyboard-triggered one, e.g. Delete) may have different purposes depending on
 * which panel is "focused" at any given time. Pressing Delete on the Timeline is not the same as pressing Delete in
 * the Project panel, for example. This kind of "focus" is slightly different from standard QWidget focus, since it
 * aims to be less specific than a single QPushButton or QLineEdit, and rather specific to the panel widgets like
 * that belong to.
 *
 * PanelFocusManager's SLOT(FocusChanged()) connects to the QApplication instance's SIGNAL(focusChanged()) so that
 * it always knows when focus has changed within the application.
 */
class PanelFocusManager : public QObject
{
  Q_OBJECT
public:
  PanelFocusManager(QObject* parent);

  /**
   * @brief Return the currently focused widget, or nullptr if nothing is focused
   */
  PanelWidget* CurrentlyFocused() const;

  /**
   * @brief Get most recently focused panel of a certain type
   *
   * @return
   *
   * The most recently focused panel of the specified type, or nullptr if none exists
   */
  template<class T>
  T* MostRecentlyFocused();

public slots:
  /**
   * @brief Connect this to a QApplication's SIGNAL(focusChanged())
   *
   * Interprets focus information to determine the currently focused panel
   */
  void FocusChanged(QWidget* old, QWidget* now);

private:
  /**
   * @brief History array for traversing through (see MostRecentlyFocused())
   */
  QList<PanelWidget*> focus_history_;
};

template<class T>
T* PanelFocusManager::MostRecentlyFocused()
{
  T* cast_test;

  for (int i=0;i<focus_history_.size();i++) {
    cast_test = dynamic_cast<T*>(focus_history_.at(i));

    if (cast_test != nullptr) {
      return cast_test;
    }
  }

  return nullptr;
}

namespace olive {
extern PanelFocusManager* panel_focus_manager;
}

#endif // PANELFOCUSMANAGER_H
