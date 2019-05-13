#include "colorinput.h"

ColorInput::ColorInput(Node *parent, const QString& id, const QString& name, bool savable, bool keyframable) :
  NodeIO(parent, id, name, savable, keyframable)
{
  AddField(new ColorField(this));

  AddAcceptedNodeInput(olive::nodes::kColor);
}

QColor ColorInput::GetColorAt(double timecode)
{
  return static_cast<ColorField*>(Field(0))->GetColorAt(timecode);
}
