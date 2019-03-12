#include "buttonfield.h"

ButtonField::ButtonField(EffectRow *parent, const QString &string) :
  EffectField(parent, nullptr, EFFECT_FIELD_UI)
{
  SetValueAt(0, string);
}

void ButtonField::SetCheckable(bool c)
{
  checkable_ = c;
}

void ButtonField::SetChecked(bool c)
{
  checked_ = c;
  emit CheckedChanged(c);
}
