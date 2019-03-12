#include "doublefield.h"

DoubleField::DoubleField(EffectRow* parent, const QString& id) :
  EffectField(parent, id, EFFECT_FIELD_DOUBLE),
  min_(qSNaN()),
  max_(qSNaN()),
  default_(0),
  display_type_(LabelSlider::LABELSLIDER_NORMAL),
  frame_rate_(30),
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

QWidget *DoubleField::CreateWidget()
{
  LabelSlider* ls = new LabelSlider();

  if (!qIsNaN(min_)) {
    ls->set_minimum_value(min_);
  }
  ls->set_default_value(default_);
  if (!qIsNaN(max_)) {
    ls->set_maximum_value(max_);
  }
  ls->set_display_type(display_type_);
  ls->set_frame_rate(frame_rate_);

  ls->setEnabled(IsEnabled());

  connect(ls, SIGNAL(valueChanged(double)), this, SLOT(UpdateFromWidget(double)));
  connect(ls, SIGNAL(clicked()), this, SIGNAL(Clicked()));
  connect(this, SIGNAL(EnabledChanged(bool)), ls, SLOT(setEnabled(bool)));

  return ls;
}

void DoubleField::ValueHasBeenSet()
{
  value_set_ = true;
}

void DoubleField::UpdateFromWidget(double d)
{
  SetValueAt(Now(), d);
}
