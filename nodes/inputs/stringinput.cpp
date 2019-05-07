#include "stringinput.h"

StringInput::StringInput(OldEffectNode* parent, const QString& id, const QString& name, bool rich_text, bool savable, bool keyframable) :
  NodeIO(parent, id, name, savable, keyframable)
{
  AddField(new StringField(this, rich_text));

  AddAcceptedNodeInput(olive::nodes::kString);
}

QString StringInput::GetStringAt(double timecode)
{
  return static_cast<StringField*>(Field(0))->GetStringAt(timecode);
}
