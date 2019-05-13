#include "buttonwidget.h"

ButtonWidget::ButtonWidget(Node* parent, const QString& name, const QString& text) :
  NodeIO(parent, nullptr, name, false, false)
{
  ButtonField* button_field = new ButtonField(this, text);

  connect(button_field, SIGNAL(CheckedChanged(bool)), this, SIGNAL(CheckedChanged(bool)));
  connect(button_field, SIGNAL(Toggled(bool)), this, SIGNAL(Toggled(bool)));
  connect(this, SLOT(SetChecked(bool)), button_field, SLOT(SetChecked(bool)));

  AddField(button_field);
}

void ButtonWidget::SetCheckable(bool c)
{
  static_cast<ButtonField*>(Field(0))->SetCheckable(c);
}

void ButtonWidget::SetChecked(bool c)
{
  static_cast<ButtonField*>(Field(0))->SetChecked(c);
}
