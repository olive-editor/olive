#include "boolinput.h"

BoolInput::BoolInput(Effect* parent, const QString& id, const QString& name, bool savable, bool keyframable) :
  EffectRow(parent, id, name, savable, keyframable)
{
  BoolField* bool_field = new BoolField(this);
  connect(bool_field, SIGNAL(Toggled(bool)), this, SIGNAL(Toggled(bool)));
  AddField(bool_field);

  AddNodeInput(olive::nodes::kBoolean);
}

bool BoolInput::GetBoolAt(double timecode)
{
  return static_cast<BoolField*>(Field(0))->GetBoolAt(timecode);
}
