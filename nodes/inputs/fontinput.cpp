#include "fontinput.h"

FontInput::FontInput(OldEffectNode* parent, const QString& id, const QString& name, bool savable, bool keyframable) :
  NodeIO(parent, id, name, savable, keyframable)
{
  AddField(new FontField(this));

  AddAcceptedNodeInput(olive::nodes::kFont);
}

QString FontInput::GetFontAt(double timecode)
{
  return static_cast<FontField*>(Field(0))->GetFontAt(timecode);
}
