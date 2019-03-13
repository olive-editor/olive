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

void BoolField::UpdateWidgetValue(QWidget *widget, double timecode)
{
  QCheckBox* cb = static_cast<QCheckBox*>(widget);

  // Setting the checked state on the checkbox below normally triggers a change() signal that will then trickle back
  // to setting a value on this field. Therefore we block its signals while we're setting this.

  cb->blockSignals(true);

  cb->setChecked(GetBoolAt(timecode));

  cb->blockSignals(false);
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
