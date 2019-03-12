#include "buttonfield.h"

#include <QPushButton>

ButtonField::ButtonField(EffectRow *parent, const QString &string) :
  EffectField(parent, nullptr, EFFECT_FIELD_UI),
  button_text_(string)
{}

void ButtonField::SetCheckable(bool c)
{
  checkable_ = c;
}

void ButtonField::SetChecked(bool c)
{
  checked_ = c;
  emit CheckedChanged(c);
}

QWidget *ButtonField::CreateWidget()
{
  QPushButton* button = new QPushButton();

  button->setCheckable(checkable_);
  button->setEnabled(IsEnabled());
  button->setText(button_text_);

  connect(this, SIGNAL(CheckedChanged(bool)), button, SLOT(setChecked(bool)));
  connect(this, SIGNAL(EnabledChanged(bool)), button, SLOT(setEnabled(bool)));
  connect(button, SIGNAL(clicked(bool)), this, SIGNAL(Clicked()));
  connect(button, SIGNAL(toggled(bool)), this, SLOT(SetChecked(bool)));
  connect(button, SIGNAL(toggled(bool)), this, SIGNAL(Toggled(bool)));

  return button;
}
