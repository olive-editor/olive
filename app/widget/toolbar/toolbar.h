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

#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QPushButton>

#include "widget/flowlayout/flowlayout.h"
#include "widget/toolbar/toolbarbutton.h"
#include "tool/tool.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A widget containing buttons for all of Olive's application-wide tools.
 *
 * Buttons are displayed in a FlowLayout that
 * adjusts and wraps (like text) depending on the widget's size.
 *
 * By default, this Toolbar is not connected to anything. It's recommended to connect SLOT(SetTool()) and
 * SIGNAL(ToolChanged()) to Core (corresponding SIGNAL(ToolChanged()) and SLOT(SetTool()) respectively) so that the
 * Toolbar updates the current tool application-wide, and is also automatically updated when the tool is changed
 * elsewhere.
 */
class Toolbar : public QWidget
{
  Q_OBJECT
public:
  /**
   * @brief Toolbar Constructor
   *
   * Creates and connects all the Toolbar buttons
   *
   * @param parent
   *
   * QWidget parent.
   */
  Toolbar(QWidget* parent);

public slots:
  /**
   * @brief Set the tool to be displayed as "selected"
   *
   * This function does not set the application-wide tool, it only sets which tool show as selected in this widget.
   * It's recommended to use this function only as a slot connected to Core::ToolChanged() so that it automatically
   * updates whenever the application-wide tool is changed.
   *
   * @param tool
   *
   * Tool to show as selected
   */
  void SetTool(const Tool::Item &tool);

  /**
   * @brief Set snapping checked value
   *
   * Similar to SetTool(), this does not set anything application-wide, it only changes the displayed button appearance.
   * In this case, whether the snapping button should show as snapping enabled or disabled.
   *
   * @param snapping
   */
  void SetSnapping(const bool &snapping);

protected:
  /**
   * @brief Qt changeEvent
   *
   * Overridden to catch language change events (see Retranslate())
   *
   * @param e
   */
  virtual void changeEvent(QEvent* e) override;

signals:
  /**
   * @brief Emitted whenever a tool is selected using this widget
   *
   * @param t
   *
   * Tool that was selected
   */
  void ToolChanged(const Tool::Item& t);

  /**
   * @brief Emitted whenever the snapping setting is changed
   *
   * @param b
   *
   * New snapping enabled setting
   */
  void SnappingChanged(const bool& b);

  /**
   * @brief Emitted when the addable object is changed from the add tool menu
   */
  void AddableObjectChanged(const Tool::AddableObject& obj);

private:
  /**
   * @brief Reset all strings based on the currently selected language
   */
  void Retranslate();

  /**
   * @brief Update icons after a style change
   */
  void UpdateIcons();

  /**
   * @brief Internal convenience function for creating tool buttons quickly
   *
   * This function will create a ToolbarButton object, set the icon to `icon`, set its tool value to `tool`, add it to
   * the widget layout, add it to toolbar_btns_ so the buttons can be iterated (done in various functions), and connect
   * the button to ToolButtonClicked().
   *
   * If you need a non-tool but similarly styled button, use CreateNonToolButton().
   *
   * @return
   *
   * The created ToolbarButton. The button parent is automatically set to `this`.
   */
  ToolbarButton* CreateToolButton(const Tool::Item& tool);

  /**
   * @brief Internal convenience function for creating buttons quickly
   *
   * Similar to CreateToolButton() but doesn't add the button to toolbar_btns_ and doesn't connect the button to
   * ToolButtonClicked(). This is to create a button that is similarly styled but doesn't actually represent a tool
   * per se.
   *
   * @return
   *
   * The created ToolbarButton. The button parent is automatically set to `this`.
   */
  ToolbarButton* CreateNonToolButton();

  /**
   * @brief Internal layout used for buttons
   */
  FlowLayout* layout_;

  /**
   * @brief Array/list of toolbar buttons
   *
   * This list is automatically appended by CreateToolButton(). It's used to iterate through the toolbar buttons
   * quickly with for loops, etc. See SetTool() for example usage.
   */
  QList<ToolbarButton*> toolbar_btns_;

  ToolbarButton* btn_pointer_tool_;
  ToolbarButton* btn_edit_tool_;
  ToolbarButton* btn_ripple_tool_;
  ToolbarButton* btn_rolling_tool_;
  ToolbarButton* btn_razor_tool_;
  ToolbarButton* btn_slip_tool_;
  ToolbarButton* btn_slide_tool_;
  ToolbarButton* btn_hand_tool_;
  ToolbarButton* btn_transition_tool_;
  ToolbarButton* btn_zoom_tool_;
  ToolbarButton* btn_record_;
  ToolbarButton* btn_add_;

  ToolbarButton* btn_snapping_toggle_;

private slots:
  /**
   * @brief Slot for a ToolbarButton being clicked
   *
   * ToolbarButtons created from CreateToolButton() are automatically connected to this slot.
   * This slot will receive the ToolbarButton's tool value
   * and emit a signal indicating that the tool has changed to the newly selected tool. This function static_casts
   * the sender to ToolbarButton so you should not connect any other class type to this slot.
   */
  void ToolButtonClicked();

  /**
   * @brief Receiver for the snapping toggle button
   *
   * This function's primary purpose is to emit SnappingChanged() and should be connected to the ToolbarButton's
   * SIGNAL(clicked(bool)).
   *
   * @param b
   *
   * The new snapping value received from the sender's clicked signal
   */
  void SnappingButtonClicked(bool b);

  /**
   * @brief Receiver for the add button
   *
   * The add button pops up a list for which object to create.
   */
  void AddButtonClicked();

  /**
   * @brief Receiver for the menu created by AddButtonClicked()
   */
  void AddMenuItemTriggered(QAction* a);

};

OLIVE_NAMESPACE_EXIT

#endif // TOOLBAR_H
