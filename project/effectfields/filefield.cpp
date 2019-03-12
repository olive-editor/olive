#include "filefield.h"

FileField::FileField(EffectRow* parent, const QString &id) :
  EffectField(parent, id, EFFECT_FIELD_FILE)
{

}

QString FileField::GetFileAt(double timecode)
{
  return GetValueAt(timecode).toString();
}
