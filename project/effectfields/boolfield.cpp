#include "boolfield.h"

BoolField::BoolField(EffectRow *parent, const QString &id) :
  EffectField(parent, id, EFFECT_FIELD_BOOL)
{}

bool BoolField::GetBoolAt(double timecode)
{
  return GetValueAt(timecode).toBool();
}

QVariant BoolField::ConvertStringToValue(const QString &s)
{
  return (s == "1");
}

QString BoolField::ConvertValueToString(const QVariant &v)
{
  return QString::number(v.toBool());
}
