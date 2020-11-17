/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "stringslider.h"

namespace olive {

StringSlider::StringSlider(QWidget* parent) :
  SliderBase(kString, parent)
{
  connect(this, SIGNAL(ValueChanged(QVariant)), this, SLOT(ConvertValue(QVariant)));
}

QString StringSlider::GetValue()
{
  return Value().toString();
}

void StringSlider::SetValue(const QString &v)
{
  SliderBase::SetValue(v);
}

QString StringSlider::ValueToString(const QVariant &v)
{
  QString vstr = v.toString();
  return (vstr.isEmpty()) ? tr("(none)") : vstr;
}

void StringSlider::ConvertValue(QVariant v)
{
  emit ValueChanged(v.toString());
}

}
