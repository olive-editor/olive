#include "fontfield.h"

FontField::FontField(EffectRow* parent, const QString &id) :
  EffectField(parent, id, EFFECT_FIELD_FONT)
{

}

QString FontField::GetFontAt(double timecode)
{
  return GetValueAt(timecode).toString();
}
