/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "toolbar.h"

#include <QPushButton>
#include <QVariant>
#include <QButtonGroup>
#include <QEvent>

#include "node/factory.h"
#include "ui/icons/icons.h"
#include "widget/menu/menu.h"

namespace olive {

Toolbar::Toolbar(QWidget *parent) :
  QWidget(parent)
{
  layout_ = new FlowLayout(this);
  layout_->setMargin(0);

  // Create standard tool buttons
  btn_pointer_tool_ = CreateToolButton(Tool::kPointer);
  btn_edit_tool_ = CreateToolButton(Tool::kEdit);
  btn_ripple_tool_ = CreateToolButton(Tool::kRipple);
  btn_rolling_tool_ = CreateToolButton(Tool::kRolling);
  btn_razor_tool_ = CreateToolButton(Tool::kRazor);
  btn_slip_tool_ = CreateToolButton(Tool::kSlip);
  btn_slide_tool_ = CreateToolButton(Tool::kSlide);
  btn_hand_tool_ = CreateToolButton(Tool::kHand);
  btn_zoom_tool_ = CreateToolButton(Tool::kZoom);
  btn_record_ = CreateToolButton(Tool::kRecord);
  btn_transition_tool_ = CreateToolButton(Tool::kTransition);
  btn_add_ = CreateToolButton(Tool::kAdd);

  // Create snapping button, which is not actually a tool, it's a toggle option
  btn_snapping_toggle_ = CreateNonToolButton();
  connect(btn_snapping_toggle_, &QPushButton::clicked, this, &Toolbar::SnappingButtonClicked);

  // Connect transition button to menu signal
  connect(btn_transition_tool_, &QPushButton::clicked, this, &Toolbar::TransitionButtonClicked);

  // Connect add button to menu signal
  connect(btn_add_, &QPushButton::clicked, this, &Toolbar::AddButtonClicked);

  Retranslate();
  UpdateIcons();
}

void Toolbar::SetTool(const Tool::Item& tool)
{
  // For each tool, set the "checked" state to whether the button's tool is the current tool
  for (int i=0;i<toolbar_btns_.size();i++) {
    ToolbarButton* btn = toolbar_btns_.at(i);

    btn->setChecked(btn->tool() == tool);
  }
}

void Toolbar::SetSnapping(const bool& snapping)
{
  // Set checked state of snapping toggle
  btn_snapping_toggle_->setChecked(snapping);
}

void Toolbar::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  } else if (e->type() == QEvent::StyleChange) {
    UpdateIcons();
  }
  QWidget::changeEvent(e);
}

void Toolbar::Retranslate()
{
  btn_pointer_tool_->setToolTip(tr("Pointer Tool"));
  btn_edit_tool_->setToolTip(tr("Edit Tool"));
  btn_ripple_tool_->setToolTip(tr("Ripple Tool"));
  btn_rolling_tool_->setToolTip(tr("Rolling Tool"));
  btn_razor_tool_->setToolTip(tr("Razor Tool"));
  btn_slip_tool_->setToolTip(tr("Slip Tool"));
  btn_slide_tool_->setToolTip(tr("Slide Tool"));
  btn_hand_tool_->setToolTip(tr("Hand Tool"));
  btn_zoom_tool_->setToolTip(tr("Zoom Tool"));
  btn_transition_tool_->setToolTip(tr("Transition Tool"));
  btn_record_->setToolTip(tr("Record Tool"));
  btn_add_->setToolTip(tr("Add Tool"));
  btn_snapping_toggle_->setToolTip(tr("Toggle Snapping"));
}

void Toolbar::UpdateIcons()
{
  btn_pointer_tool_->setIcon(icon::ToolPointer);
  btn_edit_tool_->setIcon(icon::ToolEdit);
  btn_ripple_tool_->setIcon(icon::ToolRipple);
  btn_rolling_tool_->setIcon(icon::ToolRolling);
  btn_razor_tool_->setIcon(icon::ToolRazor);
  btn_slip_tool_->setIcon(icon::ToolSlip);
  btn_slide_tool_->setIcon(icon::ToolSlide);
  btn_hand_tool_->setIcon(icon::ToolHand);
  btn_zoom_tool_->setIcon(icon::ZoomIn);
  btn_record_->setIcon(icon::Record);
  btn_transition_tool_->setIcon(icon::ToolTransition);
  btn_add_->setIcon(icon::Add);
  btn_snapping_toggle_->setIcon(icon::Snapping);
}

ToolbarButton* Toolbar::CreateToolButton(const Tool::Item& tool)
{
  // Create a ToolbarButton object
  ToolbarButton* b = new ToolbarButton(this, tool);

  // Add it to the layout
  layout_->addWidget(b);

  // Add it to the list for iterating through later
  toolbar_btns_.append(b);

  // Connect it to the tool button click handler
  connect(b, SIGNAL(clicked(bool)), this, SLOT(ToolButtonClicked()));

  return b;
}

ToolbarButton *Toolbar::CreateNonToolButton()
{
  // Create a ToolbarButton object
  ToolbarButton* b = new ToolbarButton(this, Tool::kNone);

  // Add it to the layout
  layout_->addWidget(b);

  return b;
}

void Toolbar::ToolButtonClicked()
{
  // Get new tool from ToolbarButton object
  Tool::Item new_tool = static_cast<ToolbarButton*>(sender())->tool();

  // Set checked state of all tool buttons
  // NOTE: Not necessary if this is appropriately connected to Core
  //SetTool(new_tool);

  // Emit signal that the tool just changed
  emit ToolChanged(new_tool);
}

void Toolbar::SnappingButtonClicked(bool b)
{
  emit SnappingChanged(b);
}

void Toolbar::AddButtonClicked()
{
  Menu m(this);

  for (int i=0;i<Tool::kAddableCount;i++) {
    QAction* action = m.addAction(Tool::GetAddableObjectName(static_cast<Tool::AddableObject>(i)));
    action->setData(i);
  }

  connect(&m, &QMenu::triggered, this, &Toolbar::AddMenuItemTriggered);

  m.exec(QCursor::pos());
}

void Toolbar::TransitionButtonClicked()
{
  Menu* m = NodeFactory::CreateMenu(this, false, Node::kCategoryTransition);

  connect(m, &QMenu::triggered, this, &Toolbar::TransitionMenuItemTriggered);

  m->exec(QCursor::pos());

  delete m;
}

void Toolbar::AddMenuItemTriggered(QAction* a)
{
  emit AddableObjectChanged(static_cast<Tool::AddableObject>(a->data().toInt()));
}

void Toolbar::TransitionMenuItemTriggered(QAction *a)
{
  emit SelectedTransitionChanged(NodeFactory::GetIDFromMenuAction(a));
}

}
