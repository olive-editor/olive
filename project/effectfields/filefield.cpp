#include "filefield.h"

FileField::FileField(EffectRow* parent, const QString &id) :
  EffectField(parent, id, EFFECT_FIELD_FILE)
{

}

QString FileField::GetFileAt(double timecode)
{
  return GetValueAt(timecode).toString();
}

QVariant FileField::ConvertStringToValue(const QString &s)
{
  return s;
}

QString FileField::ConvertValueToString(const QVariant &v)
{
  return v.toString();
}
