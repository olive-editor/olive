#include "labelfield.h"

#include <QLabel>

LabelField::LabelField(EffectRow *parent, const QString &string) :
  EffectField(parent, nullptr, EFFECT_FIELD_UI),
  label_text_(string)
{}

QWidget *LabelField::CreateWidget(QWidget *existing)
{
  QLabel* label;

  if (existing == nullptr) {

    label = new QLabel(label_text_);

    label->setEnabled(IsEnabled());

  } else {

    label = static_cast<QLabel*>(existing);

  }

  connect(this, SIGNAL(EnabledChanged(bool)), label, SLOT(setEnabled(bool)));

  return label;
}
