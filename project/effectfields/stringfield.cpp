#include "stringfield.h"

StringField::StringField(EffectRow* parent, const QString& id) :
  EffectField(parent, id, EFFECT_FIELD_STRING)
{

}

QString StringField::GetStringAt(double timecode)
{
  return GetValueAt(timecode).toString();
}
