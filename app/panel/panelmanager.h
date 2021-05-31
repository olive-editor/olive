/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "widget/panel/panel.h"

namespace olive {

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
class PanelManager : public QObject
{
  Q_OBJECT
public:
  PanelManager(QObject* parent = nullptr);

  /**
   * @brief Destroy all panels
   *
   * Should only be used on application exit to cleanly free all panels.
   */
  void DeleteAllPanels();

  /**
   * @brief Get a list of all existing panels
   *
   * Panels are ordered from most recently focused to least recently focused.
   */
  const QList<PanelWidget*>& panels();

  /**
   * @brief Return the currently focused widget, or nullptr if nothing is focused
   *
   * This result == CurrentlyFocused() if HoverFocus is true
   */
  PanelWidget* CurrentlyFocused() const;

  /**
   * @brief Return the widget that the mouse is currently hovering over, or nullptr if nothing is hovered over
   *
   * This result == CurrentlyFocused() if HoverFocus is true
   */
  PanelWidget* CurrentlyHovered() const;

  template<class T>
  /**
   * @brief Get most recently focused panel of a certain type
   *
   * @return
   *
   * The most recently focused panel of the specified type, or nullptr if none exists
   */
  T* MostRecentlyFocused();

  template<class T>
  /**
   * @brief Create a panel
   */
  T* CreatePanel(QWidget* parent);

  /**
   * @brief Get whether panels are currently prevented from moving
   */
  bool ArePanelsLocked();

  /**
   * @brief Create PanelManager singleton instance
   */
  static void CreateInstance();

  /**
   * @brief Destroy PanelManager singleton instance
   *
   * If no PanelManager was created, this is a no-op.
   */
  static void DestroyInstance();

  /**
   * @brief Access to PanelManager singleton instance
   */
  static PanelManager* instance();

  template<class T>
  /**
   * @brief Get a list of panels of a certain type
   */
  QList<T*> GetPanelsOfType();

public slots:
  /**
   * @brief Connect this to a QApplication's SIGNAL(focusChanged())
   *
   * Interprets focus information to determine the currently focused panel
   */
  void FocusChanged(QWidget* old, QWidget* now);

  /**
   * @brief Sets whether panels should be prevented from moving
   */
  void SetPanelsLocked(bool locked);

signals:
  /**
   * @brief Signal emitted when the currently focused panel changes
   */
  void FocusedPanelChanged(PanelWidget* panel);

private:
  /**
   * @brief History array for traversing through (see MostRecentlyFocused())
   */
  QList<PanelWidget*> focus_history_;

  /**
   * @brief Internal panel movement is locked value
   */
  bool locked_;

  /**
   * @brief PanelManager singleton instance
   */
  static PanelManager* instance_;

  /**
   * @brief The last panel that was focused
   *
   * Stored to prevent emitting FocusedPanelChanged() multiple times for the same panel
   */
  PanelWidget* last_focused_panel_;

private slots:
  /**
   * @brief Processing if a panel gets deleted
   */
  void PanelDestroyed();

};

template<class T>
T *PanelManager::CreatePanel(QWidget *parent)
{
  T* panel = new T(parent);

  // Add panel to the bottom of the focus history
  focus_history_.append(panel);

  panel->SetMovementLocked(locked_);

  // Sane default for panel size
  panel->resize(parent->size() / 3);

  // We're about to center the panel relative to the parent (usually the main window), but for some
  // reason this requires the panel to be shown first.
  panel->show();

  // Center the panel relative to the parent
  QPoint parent_center = panel->mapFromGlobal(parent->mapToGlobal(parent->rect().center()));
  QPoint panel_center = panel->rect().center();
  panel->move(parent_center - panel_center);

  // Connect destroy signal so we can remove it from focus history
  connect(panel, &PanelWidget::destroyed, this, &PanelManager::PanelDestroyed, Qt::DirectConnection);

  return panel;
}

template<class T>
T* PanelManager::MostRecentlyFocused()
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

template<class T>
QList<T*> PanelManager::GetPanelsOfType()
{
  QList<T*> panels;

  T* cast_test;

  foreach (PanelWidget* panel, focus_history_) {
    cast_test = dynamic_cast<T*>(panel);

    if (cast_test) {
      panels.append(cast_test);
    }
  }

  return panels;
}

}

#endif // PANELFOCUSMANAGER_H
