#include "colorfield.h"

#include <QColor>

#include "ui/colorbutton.h"

ColorField::ColorField(EffectRow* parent, const QString& id) :
  EffectField(parent, id, EFFECT_FIELD_COLOR)
{}

QColor ColorField::GetColorAt(double timecode)
{
  return GetValueAt(timecode).value<QColor>();
}

QWidget *ColorField::CreateWidget(QWidget *existing)
{
  ColorButton* cb = (existing != nullptr) ? static_cast<ColorButton*>(existing) : new ColorButton();

  connect(cb, SIGNAL(color_changed(const QColor &)), this, SLOT(UpdateFromWidget(const QColor &)));
  connect(this, SIGNAL(EnabledChanged(bool)), cb, SLOT(setEnabled(bool)));

  return cb;
}

void ColorField::UpdateWidgetValue(QWidget *widget, double timecode)
{
  ColorButton* cb = static_cast<ColorButton*>(widget);

  cb->set_color(GetColorAt(timecode));
}

QVariant ColorField::ConvertStringToValue(const QString &s)
{
  return QColor(s);
}

QString ColorField::ConvertValueToString(const QVariant &v)
{
  return v.value<QColor>().name();
}

void ColorField::UpdateFromWidget(const QColor& c)
{
  KeyframeDataChange* kdc = new KeyframeDataChange(this);

  SetValueAt(Now(), c);

  kdc->SetNewKeyframes();
  olive::UndoStack.push(kdc);
}
