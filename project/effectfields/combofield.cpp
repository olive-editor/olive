#include "combofield.h"

#include <QComboBox>

ComboField::ComboField(EffectRow* parent, const QString& id) :
  EffectField(parent, id, EFFECT_FIELD_COMBO)
{}

void ComboField::AddItem(const QString &text, const QVariant &data)
{
  ComboFieldItem item;

  item.name = text;
  item.data = data;

  items_.append(item);
}

QWidget *ComboField::CreateWidget()
{
  QComboBox* cb = new QComboBox();

  for (int i=0;i<items_.size();i++) {
    cb->addItem(items_.at(i).name);
  }

  connect(cb, SIGNAL(activated(int)), this, SLOT(UpdateFromWidget(int)));

  return cb;
}

void ComboField::UpdateFromWidget(int index)
{
  SetValueAt(Now(), items_.at(index).data);
}
