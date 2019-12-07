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

#include "integerslider.h"

IntegerSlider::IntegerSlider(QWidget* parent) :
  SliderBase(kInteger, parent)
{
  connect(this, SIGNAL(ValueChanged(QVariant)), this, SLOT(ConvertValue(QVariant)));
}

int IntegerSlider::GetValue()
{
  return Value().toInt();
}

void IntegerSlider::SetValue(const int64_t &v)
{
  SliderBase::SetValue(v);
}

void IntegerSlider::SetMinimum(const int64_t &d)
{
  SetMinimumInternal(d);
}

void IntegerSlider::SetMaximum(const int64_t &d)
{
  SetMaximumInternal(d);
}

QVariant IntegerSlider::StringToValue(const QString &s, bool *ok)
{
  bool valid;

  // Allow both floats and integers for either modes
  double decimal_val = s.toDouble(&valid);

  if (ok) {
    *ok = valid;
  }

  if (valid) {
    // But for an integer, we round it
    return qRound(decimal_val);
  }

  return QVariant();
}

void IntegerSlider::ConvertValue(QVariant v)
{
  emit ValueChanged(v.toInt());
}
