#include "toolbar.h"

#include <QPushButton>
#include <QEvent>

#include "widget/flowlayout/flowlayout.h"
#include "ui/icons/icons.h"

Toolbar::Toolbar(QWidget *parent) :
  QWidget(parent)
{
  FlowLayout* layout = new FlowLayout(this);
  layout->setMargin(0);

  btn_pointer_tool = new QPushButton();
  btn_pointer_tool->setIcon(olive::icon::ToolPointer);
  layout->addWidget(btn_pointer_tool);

  btn_edit_tool = new QPushButton();
  btn_edit_tool->setIcon(olive::icon::ToolEdit);
  layout->addWidget(btn_edit_tool);

  btn_ripple_tool = new QPushButton();
  btn_ripple_tool->setIcon(olive::icon::ToolRipple);
  layout->addWidget(btn_ripple_tool);

  btn_razor_tool = new QPushButton();
  btn_razor_tool->setIcon(olive::icon::ToolRazor);
  layout->addWidget(btn_razor_tool);

  btn_slip_tool = new QPushButton();
  btn_slip_tool->setIcon(olive::icon::ToolSlip);
  layout->addWidget(btn_slip_tool);

  btn_slide_tool = new QPushButton();
  btn_slide_tool->setIcon(olive::icon::ToolSlide);
  layout->addWidget(btn_slide_tool);

  btn_hand_tool = new QPushButton();
  btn_hand_tool->setIcon(olive::icon::ToolHand);
  layout->addWidget(btn_hand_tool);

  btn_transition_tool = new QPushButton();
  btn_transition_tool->setIcon(olive::icon::ToolTransition);
  layout->addWidget(btn_transition_tool);

  btn_snapping_toggle = new QPushButton();
  btn_snapping_toggle->setIcon(olive::icon::Snapping);
  layout->addWidget(btn_snapping_toggle);

  btn_zoom_in = new QPushButton();
  btn_zoom_in->setIcon(olive::icon::ZoomIn);
  layout->addWidget(btn_zoom_in);

  btn_zoom_out = new QPushButton();
  btn_zoom_out->setIcon(olive::icon::ZoomOut);
  layout->addWidget(btn_zoom_out);

  btn_record = new QPushButton();
  btn_record->setIcon(olive::icon::Record);
  layout->addWidget(btn_record);

  btn_add = new QPushButton();
  btn_add->setIcon(olive::icon::Add);
  layout->addWidget(btn_add);

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
  btn_pointer_tool->setToolTip(tr("Pointer Tool"));
  btn_edit_tool->setToolTip(tr("Edit Tool"));
  btn_ripple_tool->setToolTip(tr("Ripple Tool"));
  btn_razor_tool->setToolTip(tr("Razor Tool"));
  btn_slip_tool->setToolTip(tr("Slip Tool"));
  btn_slide_tool->setToolTip(tr("Slide Tool"));
  btn_hand_tool->setToolTip(tr("Hand Tool"));
  btn_transition_tool->setToolTip(tr("Transition Tool"));
  btn_snapping_toggle->setToolTip(tr("Toggle Snapping"));
  btn_zoom_in->setToolTip(tr("Zoom In"));
  btn_zoom_out->setToolTip(tr("Zoom Out"));
  btn_record->setToolTip(tr("Record Audio"));
  btn_add->setToolTip(tr("Add Object"));
}
