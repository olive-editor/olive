#include "comboinput.h"

ComboInput::ComboInput(Node* parent, const QString& id, const QString& name, bool savable, bool keyframable) :
  EffectRow(parent, id, name, savable, keyframable)
{
  ComboField* combo_field = new ComboField(this);
  connect(combo_field, SIGNAL(DataChanged(const QVariant&)), this, SIGNAL(DataChanged(const QVariant&)));
  AddField(combo_field);

  AddAcceptedNodeInput(olive::nodes::kCombo);
}

void ComboInput::AddItem(const QString &text, const QVariant &data)
{
  static_cast<ComboField*>(Field(0))->AddItem(text, data);
}
