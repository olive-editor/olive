#include "colorinput.h"

ColorInput::ColorInput(Effect* parent, const QString& id, const QString& name, bool savable, bool keyframable) :
  EffectRow(parent, id, name, savable, keyframable)
{
  AddField(new ColorField(this));
}

QColor ColorInput::GetColorAt(double timecode)
{
  static_cast<ColorField*>(Field(0))->GetColorAt(timecode);
}
