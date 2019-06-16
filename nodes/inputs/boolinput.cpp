#include "boolinput.h"

BoolInput::BoolInput(Node *parent, const QString& id, const QString& name, bool savable, bool keyframable) :
  NodeIO(parent, id, name, savable, keyframable)
{
  BoolField* bool_field = new BoolField(this);
  connect(bool_field, SIGNAL(Toggled(bool)), this, SIGNAL(Toggled(bool)));
  AddField(bool_field);

  AddAcceptedNodeInput(olive::nodes::kBoolean);
}

bool BoolInput::GetBoolAt(const rational& timecode)
{
  return static_cast<BoolField*>(Field(0))->GetBoolAt(timecode);
}
