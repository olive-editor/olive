#include "labelfield.h"

#include <QLabel>

LabelField::LabelField(EffectRow *parent, const QString &string) :
  EffectField(parent, nullptr, EFFECT_FIELD_UI),
  label_text_(string)
{}

QWidget *LabelField::CreateWidget()
{
  QLabel* label = new QLabel(label_text_);

  label->setEnabled(IsEnabled());
  connect(this, SIGNAL(EnabledChanged(bool)), label, SLOT(setEnabled(bool)));

  return label;
}
