#include "boolfield.h"

#include <QCheckBox>

BoolField::BoolField(EffectRow *parent, const QString &id) :
  EffectField(parent, id, EFFECT_FIELD_BOOL)
{}

bool BoolField::GetBoolAt(double timecode)
{
  return GetValueAt(timecode).toBool();
}

QWidget *BoolField::CreateWidget()
{
  QCheckBox* cb = new QCheckBox();

  connect(cb, SIGNAL(toggled(bool)), this, SLOT(UpdateFromWidget(bool)));
  connect(this, SIGNAL(EnabledChanged(bool)), cb, SLOT(setEnabled(bool)));
  connect(cb, SIGNAL(toggled(bool)), this, SIGNAL(Toggled(bool)));

  return cb;
}

QVariant BoolField::ConvertStringToValue(const QString &s)
{
  return (s == "1");
}

QString BoolField::ConvertValueToString(const QVariant &v)
{
  return QString::number(v.toBool());
}

void BoolField::UpdateFromWidget(bool b)
{
  SetValueAt(Now(), b);
}
