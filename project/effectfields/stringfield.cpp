#include "stringfield.h"

StringField::StringField(EffectRow* parent, const QString& id) :
  EffectField(parent, id, EFFECT_FIELD_STRING)
{

}

QString StringField::GetStringAt(double timecode)
{
  return GetValueAt(timecode).toString();
}

QVariant StringField::ConvertStringToValue(const QString &s)
{
  return s;
}

QString StringField::ConvertValueToString(const QVariant &v)
{
  return v.toString();
}
