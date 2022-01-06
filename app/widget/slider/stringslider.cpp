/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#define super SliderBase

StringSlider::StringSlider(QWidget* parent) :
  super(parent)
{
  SetValue(QString());

  connect(label(), &SliderLabel::LabelReleased, this, &SliderBase::ShowEditor);
}

QString StringSlider::GetValue() const
{
  return GetValueInternal().toString();
}

void StringSlider::SetValue(const QString &v)
{
  SetValueInternal(v);
}

void StringSlider::SetDefaultValue(const QString &v)
{
  super::SetDefaultValue(v);
}

QString StringSlider::ValueToString(const QVariant &v) const
{
  QString vstr = v.toString();
  return (vstr.isEmpty()) ? tr("(none)") : vstr;
}

QVariant StringSlider::StringToValue(const QString &s, bool *ok) const
{
  *ok = true;
  return s;
}

void StringSlider::ValueSignalEvent(const QVariant &value)
{
  emit ValueChanged(value.toString());
}

}
