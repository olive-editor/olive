#include "toolbar.h"

#include <QPushButton>
#include <QButtonGroup>
#include <QEvent>

#include "ui/icons/icons.h"

Toolbar::Toolbar(QWidget *parent) :
  QWidget(parent)
{
  layout_ = new FlowLayout(this);
  layout_->setMargin(0);

  btn_pointer_tool_ = CreateToolButton(olive::icon::ToolPointer);
  btn_edit_tool_ = CreateToolButton(olive::icon::ToolEdit);
  btn_ripple_tool_ = CreateToolButton(olive::icon::ToolRipple);
  btn_razor_tool_ = CreateToolButton(olive::icon::ToolRazor);
  btn_slip_tool_ = CreateToolButton(olive::icon::ToolSlip);
  btn_slide_tool_ = CreateToolButton(olive::icon::ToolSlide);
  btn_hand_tool_ = CreateToolButton(olive::icon::ToolHand);
  btn_zoom_tool_ = CreateToolButton(olive::icon::ZoomIn);
  btn_transition_tool_ = CreateToolButton(olive::icon::ToolTransition);
  btn_snapping_toggle_ = CreateToolButton(olive::icon::Snapping);
  btn_record_ = CreateToolButton(olive::icon::Record);
  btn_add_ = CreateToolButton(olive::icon::Add);

  Retranslate();
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

QPushButton* Toolbar::CreateToolButton(const QIcon &icon)
{
  QPushButton* b = new QPushButton();
  b->setIcon(icon);
  b->setCheckable(true);
  layout_->addWidget(b);
  return b;
}
