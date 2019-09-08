/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "floatslider.h"

FloatSlider::FloatSlider(QWidget *parent) :
  SliderBase(kFloat, parent)
{
  connect(this, SIGNAL(ValueChanged(QVariant)), this, SLOT(ConvertValue(QVariant)));
}

double FloatSlider::GetValue()
{
  return Value().toDouble();
}

void FloatSlider::SetValue(const double &d)
{
  SliderBase::SetValue(d);
}

void FloatSlider::SetMinimum(const double &d)
{
  SetMinimumInternal(d);
}

void FloatSlider::SetMaximum(const double &d)
{
  SetMaximumInternal(d);
}

void FloatSlider::SetDecimalPlaces(int i)
{
  decimal_places_ = i;

  UpdateLabel(Value());
}

void FloatSlider::ConvertValue(QVariant v)
{
  emit ValueChanged(v.toDouble());
}
