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

#include "toolbar.h"

#include <QPushButton>
#include <QVariant>
#include <QButtonGroup>
#include <QEvent>

#include "ui/icons/icons.h"

Toolbar::Toolbar(QWidget *parent) :
  QWidget(parent)
{
  layout_ = new FlowLayout(this);
  layout_->setMargin(0);

  // Create standard tool buttons
  btn_pointer_tool_ = CreateToolButton(olive::icon::ToolPointer, olive::tool::kPointer);
  btn_edit_tool_ = CreateToolButton(olive::icon::ToolEdit, olive::tool::kEdit);
  btn_ripple_tool_ = CreateToolButton(olive::icon::ToolRipple, olive::tool::kRipple);
  btn_razor_tool_ = CreateToolButton(olive::icon::ToolRazor, olive::tool::kRazor);
  btn_slip_tool_ = CreateToolButton(olive::icon::ToolSlip, olive::tool::kSlip);
  btn_slide_tool_ = CreateToolButton(olive::icon::ToolSlide, olive::tool::kSlide);
  btn_hand_tool_ = CreateToolButton(olive::icon::ToolHand, olive::tool::kHand);
  btn_zoom_tool_ = CreateToolButton(olive::icon::ZoomIn, olive::tool::kZoom);
  btn_transition_tool_ = CreateToolButton(olive::icon::ToolTransition, olive::tool::kTransition);
  btn_record_ = CreateToolButton(olive::icon::Record, olive::tool::kRecord);
  btn_add_ = CreateToolButton(olive::icon::Add, olive::tool::kAdd);

  // Create snapping button, which is not actually a tool, it's a toggle option
  btn_snapping_toggle_ = new ToolbarButton(this, olive::icon::Snapping, olive::tool::kNone);
  layout_->addWidget(btn_snapping_toggle_);
  connect(btn_snapping_toggle_, SIGNAL(clicked(bool)), this, SLOT(SnappingButtonClicked(bool)));

  Retranslate();
}

void Toolbar::SetTool(const olive::tool::Tool& tool)
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
  }
  QWidget::changeEvent(e);
}

void Toolbar::Retranslate()
{
  btn_pointer_tool_->setToolTip(tr("Pointer Tool"));
  btn_edit_tool_->setToolTip(tr("Edit Tool"));
  btn_ripple_tool_->setToolTip(tr("Ripple Tool"));
  btn_razor_tool_->setToolTip(tr("Razor Tool"));
  btn_slip_tool_->setToolTip(tr("Slip Tool"));
  btn_slide_tool_->setToolTip(tr("Slide Tool"));
  btn_hand_tool_->setToolTip(tr("Hand Tool"));
  btn_zoom_tool_->setToolTip(tr("Zoom Tool"));
  btn_transition_tool_->setToolTip(tr("Transition Tool"));
  btn_snapping_toggle_->setToolTip(tr("Toggle Snapping"));
  btn_record_->setToolTip(tr("Record Audio"));
  btn_add_->setToolTip(tr("Add Object"));
}

ToolbarButton* Toolbar::CreateToolButton(const QIcon &icon, const olive::tool::Tool& tool)
{
  // Create a ToolbarButton object
  ToolbarButton* b = new ToolbarButton(this, icon, tool);

  // Add it to the layout
  layout_->addWidget(b);

  // Add it to the list for iterating through later
  toolbar_btns_.append(b);

  // Connect it to the tool button click handler
  connect(b, SIGNAL(clicked(bool)), this, SLOT(ToolButtonClicked()));

  return b;
}

void Toolbar::ToolButtonClicked()
{
  // Get new tool from ToolbarButton object
  olive::tool::Tool new_tool = static_cast<ToolbarButton*>(sender())->tool();

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
