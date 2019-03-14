#include "fontfield.h"

#include <QFontDatabase>
#include <QDebug>

#include "ui/comboboxex.h"

// NOTE/TODO: This shares a lot of similarity with ComboField, and could probably be a derived class of it

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

QWidget *FontField::CreateWidget(QWidget *existing)
{
  ComboBoxEx* fcb = new ComboBoxEx();

  if (existing == nullptr) {

    fcb = new ComboBoxEx();

    fcb->setScrollingEnabled(false);

    fcb->addItems(font_list);

  } else {

    fcb = static_cast<ComboBoxEx*>(existing);

  }

  connect(fcb, SIGNAL(currentTextChanged(const QString &)), this, SLOT(UpdateFromWidget(const QString &)));
  connect(this, SIGNAL(EnabledChanged(bool)), fcb, SLOT(setEnabled(bool)));

  return fcb;
}

void FontField::UpdateWidgetValue(QWidget *widget, double timecode)
{
  QVariant data = GetValueAt(timecode);

  ComboBoxEx* cb = static_cast<ComboBoxEx*>(widget);

  for (int i=0;i<font_list.size();i++) {
    if (font_list.at(i) == data) {
      cb->blockSignals(true);
      cb->setCurrentIndex(i);
      cb->blockSignals(false);
      return;
    }
  }

  qWarning() << "Failed to set FontField value from data";
}

void FontField::UpdateFromWidget(const QString& s)
{
  KeyframeDataChange* kdc = new KeyframeDataChange(this);

  SetValueAt(Now(), s);

  kdc->SetNewKeyframes();
  olive::UndoStack.push(kdc);
}
