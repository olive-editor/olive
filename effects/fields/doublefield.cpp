/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "doublefield.h"

#include "nodes/node.h"
#include "undo/undo.h"

DoubleField::DoubleField(NodeIO* parent) :
  EffectField(parent, EffectField::EFFECT_FIELD_DOUBLE),
  min_(qSNaN()),
  max_(qSNaN()),
  default_(0),
  display_type_(LabelSlider::Normal),
  frame_rate_(30),
  value_set_(false),
  kdc_(nullptr)
{
  connect(this, SIGNAL(Changed()), this, SLOT(ValueHasBeenSet()), Qt::DirectConnection);
}

double DoubleField::GetDoubleAt(const rational& timecode)
{
  return GetValueAt(timecode).toDouble();
}

void DoubleField::SetMinimum(double minimum)
{
  min_ = minimum;
  emit MinimumChanged(min_);
}

void DoubleField::SetMaximum(double maximum)
{
  max_ = maximum;
  emit MaximumChanged(max_);
}

void DoubleField::SetDefault(double d)
{
  default_ = d;

  if (!value_set_) {
    SetValueAt(0, d);
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

QWidget *DoubleField::CreateWidget(QWidget *existing)
{
  LabelSlider* ls;

  if (existing == nullptr) {

    ls = new LabelSlider();

    if (!qIsNaN(min_)) {
      ls->SetMinimum(min_);
    }
    ls->SetDefault(default_);
    if (!qIsNaN(max_)) {
      ls->SetMaximum(max_);
    }
    ls->SetDisplayType(display_type_);
    ls->SetFrameRate(frame_rate_);

    ls->setEnabled(IsEnabled());

  } else {

    ls = static_cast<LabelSlider*>(existing);

  }

  connect(ls, SIGNAL(valueChanged(double)), this, SLOT(UpdateFromWidget(double)));
  connect(ls, SIGNAL(clicked()), this, SIGNAL(Clicked()));
  connect(this, SIGNAL(EnabledChanged(bool)), ls, SLOT(setEnabled(bool)));
  connect(this, SIGNAL(MaximumChanged(double)), ls, SLOT(SetMaximum(double)));
  connect(this, SIGNAL(MinimumChanged(double)), ls, SLOT(SetMinimum(double)));

  return ls;
}

void DoubleField::UpdateWidgetValue(QWidget *widget, const rational &timecode)
{
  //if (qIsNaN(timecode)) {
    //static_cast<LabelSlider*>(widget)->SetValue(qSNaN());
  //} else {
    static_cast<LabelSlider*>(widget)->SetValue(GetDoubleAt(timecode));
  //}
}

void DoubleField::ValueHasBeenSet()
{
  value_set_ = true;
}

void DoubleField::UpdateFromWidget(double d)
{
  LabelSlider* ls = static_cast<LabelSlider*>(sender());

  if (ls->IsDragging() && kdc_ == nullptr) {
    kdc_ = new KeyframeDataChange(this);
  }

  SetValueAt(GetParentRow()->ParentNode()->Time(), d);

  if (!ls->IsDragging() && kdc_ != nullptr) {
    kdc_->SetNewKeyframes();

    olive::undo_stack.push(kdc_);

    kdc_ = nullptr;
  }
}
