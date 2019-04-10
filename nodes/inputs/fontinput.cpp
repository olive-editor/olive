#include "fontinput.h"

FontInput::FontInput(Effect* parent, const QString& id, const QString& name, bool savable, bool keyframable) :
  EffectRow(parent, id, name, savable, keyframable)
{
  AddField(new FontField(this));
}

QString FontInput::GetFontAt(double timecode)
{
  return static_cast<FontField*>(Field(0))->GetFontAt(timecode);
}
