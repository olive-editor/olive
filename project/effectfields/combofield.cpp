#include "combofield.h"

ComboField::ComboField(EffectRow* parent, const QString& id) :
  EffectField(parent, id, EFFECT_FIELD_COMBO)
{}

void ComboField::AddItem(const QString &text, const QVariant &data)
{
  ComboFieldItem item;

  item.name = text;
  item.data.append(data);

  items_.append(item);
}

QVariant ComboField::ConvertStringToValue(const QString &s)
{
  return s;
}

QString ComboField::ConvertValueToString(const QVariant &v)
{
  return v.toString();
}
