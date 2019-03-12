#include "fontfield.h"

#include <QComboBox>
#include <QFontDatabase>

FontField::FontField(EffectRow* parent, const QString &id) :
  EffectField(parent, id, EFFECT_FIELD_FONT)
{
  font_list = QFontDatabase().families();

  SetValueAt(0, font_list.first());
}

QString FontField::GetFontAt(double timecode)
{
  return GetValueAt(timecode).toString();
}

QWidget *FontField::CreateWidget()
{
  QComboBox* fcb = new QComboBox();

  fcb->addItems(font_list);

  connect(fcb, SIGNAL(currentTextChanged(const QString &)), this, SLOT(UpdateFromWidget(const QString &)));
  connect(this, SIGNAL(EnabledChanged(bool)), fcb, SLOT(setEnabled(bool)));

  return fcb;
}

void FontField::UpdateFromWidget(const QString& s)
{
  SetValueAt(Now(), s);
}
