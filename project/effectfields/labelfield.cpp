#include "labelfield.h"

LabelField::LabelField(EffectRow *parent, const QString &string) :
  EffectField(parent, nullptr, EFFECT_FIELD_UI)
{
  SetValueAt(0, string);
}
