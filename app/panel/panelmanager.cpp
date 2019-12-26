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

#include "panelmanager.h"

#include "config/config.h"

PanelManager* PanelManager::instance_ = nullptr;

PanelManager::PanelManager(QObject *parent) :
  QObject(parent),
  locked_(false)
{
}

void PanelManager::DeleteAllPanels()
{
  foreach (PanelWidget* panel, focus_history_) {
    delete panel;
  }
  focus_history_.clear();
}

const QList<PanelWidget *> &PanelManager::panels()
{
  return focus_history_;
}

PanelWidget *PanelManager::CurrentlyFocused() const
{
  // If hover focus is enabled, find the currently hovered panel and return it (if no panel is hovered, resort to
  // default behavior)
  if (Config::Current()["HoverFocus"].toBool()) {
    PanelWidget* hovered = CurrentlyHovered();

    if (hovered != nullptr) {
      return hovered;
    }
  }

  if (focus_history_.isEmpty()) {
    return nullptr;
  }

  return focus_history_.first();
}

PanelWidget *PanelManager::CurrentlyHovered() const
{
  foreach (PanelWidget* panel, focus_history_) {
    if (panel->underMouse()) {
      return panel;
    }
  }

  return nullptr;
}

bool PanelManager::ArePanelsLocked()
{
  return locked_;
}

void PanelManager::CreateInstance()
{
  instance_ = new PanelManager();
}

void PanelManager::DestroyInstance()
{
  delete instance_;
}

PanelManager *PanelManager::instance()
{
  return instance_;
}

void PanelManager::FocusChanged(QWidget *old, QWidget *now)
{
  Q_UNUSED(old)

  QObject* parent = now;
  PanelWidget* panel_cast_test;

  // Loop through widget's parent hierarchy
  while (parent != nullptr) {

    // Use dynamic_cast to test if this object is a PanelWidget
    panel_cast_test = dynamic_cast<PanelWidget*>(parent);

    if (panel_cast_test != nullptr) {
      // If so, bump this to the top of the focus history

      int panel_index = focus_history_.indexOf(panel_cast_test);

      // Force the old panel to repaint (if there is one) so it hides its border
      if (!focus_history_.isEmpty()) {
        focus_history_.first()->SetBorderVisible(false);
      }

      // If it's not in the focus history, prepend it, otherwise move it
      if (panel_index == -1) {
        focus_history_.prepend(panel_cast_test);
      } else {
        focus_history_.move(panel_index, 0);
      }

      // Force the panel to repaint so it shows a border
      panel_cast_test->SetBorderVisible(true);

      break;
    }

    parent = parent->parent();
  }
}

void PanelManager::SetPanelsLocked(bool locked)
{
  foreach (PanelWidget* panel, focus_history_) {
    panel->SetMovementLocked(locked);
  }

  locked_ = locked;
}
