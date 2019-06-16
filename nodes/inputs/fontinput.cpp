#include "fontinput.h"

FontInput::FontInput(Node *parent, const QString& id, const QString& name, bool savable, bool keyframable) :
  NodeIO(parent, id, name, savable, keyframable)
{
  AddField(new FontField(this));

  AddAcceptedNodeInput(olive::nodes::kFont);
}

QString FontInput::GetFontAt(const rational& timecode)
{
  return static_cast<FontField*>(Field(0))->GetFontAt(timecode);
}
