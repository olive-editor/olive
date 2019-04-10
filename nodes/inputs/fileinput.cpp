#include "fileinput.h"

FileInput::FileInput(Effect* parent, const QString& id, const QString& name, bool savable, bool keyframable) :
  EffectRow(parent, id, name, savable, keyframable)
{
  AddField(new FileField(this));
}

QString FileInput::GetFileAt(double timecode)
{
  return static_cast<FileField*>(Field(0))->GetFileAt(timecode);
}
