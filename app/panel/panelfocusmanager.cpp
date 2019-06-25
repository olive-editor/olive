#include "panelfocusmanager.h"

#include <QDebug>

PanelFocusManager* olive::panel_focus_manager;

PanelFocusManager::PanelFocusManager(QObject *parent) :
  QObject(parent)
{

}

PanelWidget *PanelFocusManager::CurrentlyFocused()
{
  if (focus_history_.isEmpty()) {
    return nullptr;
  }

  return focus_history_.first();
}

void PanelFocusManager::FocusChanged(QWidget *old, QWidget *now)
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
        focus_history_.first()->update();
      }

      // If it's not in the focus history, prepend it, otherwise move it
      if (panel_index == -1) {
        focus_history_.prepend(panel_cast_test);
      } else {
        focus_history_.move(panel_index, 0);
      }

      // Force the panel to repaint so it shows a border
      panel_cast_test->update();

      break;
    }

    parent = parent->parent();
  }
}
