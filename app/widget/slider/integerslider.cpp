/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "integerslider.h"

namespace olive {

#define super NumericSliderBase

IntegerSlider::IntegerSlider(QWidget* parent) :
  super(parent)
{
  SetValue(0);
}

int64_t IntegerSlider::GetValue()
{
  return GetValueInternal().value<int64_t>();
}

void IntegerSlider::SetValue(const int64_t &v)
{
  SetValueInternal(v);
}

void IntegerSlider::SetMinimum(const int64_t &d)
{
  SetMinimumInternal(d);
}

void IntegerSlider::SetMaximum(const int64_t &d)
{
  SetMaximumInternal(d);
}

void IntegerSlider::SetDefaultValue(const int64_t &d)
{
  super::SetDefaultValue(d);
}

QString IntegerSlider::ValueToString(const InternalType &v) const
{
  return QString::number(v.value<int64_t>() + GetOffset().value<int64_t>());
}

IntegerSlider::InternalType IntegerSlider::StringToValue(const QString &s, bool *ok) const
{
  bool valid;

  // Allow both floats and integers for either modes
  double decimal_val = s.toDouble(&valid);

  if (ok) {
    *ok = valid;
  }

  decimal_val -= GetOffset().value<int64_t>();

  if (valid) {
    // But for an integer, we round it
    return qRound(decimal_val);
  }

  return InternalType();
}

void IntegerSlider::ValueSignalEvent(const InternalType &value)
{
  emit ValueChanged(value.value<int64_t>());
}

IntegerSlider::InternalType IntegerSlider::AdjustDragDistanceInternal(const InternalType &start, const double &drag) const
{
  return int64_t(super::AdjustDragDistanceInternal(start, drag).value<int64_t>());
}

bool IntegerSlider::Equals(const InternalType &a, const InternalType &b) const
{
  return a.value<int64_t>() == b.value<int64_t>();
}

}
