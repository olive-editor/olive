#include "colorfield.h"

#include <QColor>

ColorField::ColorField(EffectRow* parent, const QString& id) :
  EffectField(parent, id, EFFECT_FIELD_COLOR)
{}

QColor ColorField::GetColorAt(double timecode)
{
  return GetValueAt(timecode).value<QColor>();
}

QVariant ColorField::ConvertStringToValue(const QString &s)
{
  return QColor(s);
}

QString ColorField::ConvertValueToString(const QVariant &v)
{
  return v.value<QColor>().name();
}
