#include "doublefield.h"

DoubleField::DoubleField(EffectRow* parent, const QString& id) :
  EffectField(parent, id, EFFECT_FIELD_DOUBLE),
  min_(DBL_MIN),
  max_(DBL_MAX),
  value_set_(false)
{
  connect(this, SIGNAL(Changed()), this, SLOT(ValueHasBeenSet()), Qt::DirectConnection);
}

double DoubleField::GetDoubleAt(double timecode)
{
  return GetValueAt(timecode).toDouble();
}

void DoubleField::SetMinimum(double minimum)
{
  min_ = minimum;
}

void DoubleField::SetMaximum(double maximum)
{
  max_ = maximum;
}

void DoubleField::SetDefault(double d)
{
  default_ = d;

  if (!value_set_) {
    SetValueAt(0, d);
    value_set_ = false;
  }
}

void DoubleField::SetDisplayType(LabelSlider::DisplayType type)
{
  display_type_ = type;
}

void DoubleField::SetFrameRate(const double &rate)
{
  frame_rate_ = rate;
}

QVariant DoubleField::ConvertStringToValue(const QString &s)
{
  return s.toDouble();
}

QString DoubleField::ConvertValueToString(const QVariant &v)
{
  return QString::number(v.toDouble());
}

void DoubleField::ValueHasBeenSet()
{
  value_set_ = true;
}
