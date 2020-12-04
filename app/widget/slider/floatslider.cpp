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

#include "floatslider.h"

#include <QAudio>

namespace olive {

FloatSlider::FloatSlider(QWidget *parent) :
  SliderBase(kFloat, parent),
  display_type_(kNormal),
  decimal_places_(1),
  autotrim_decimal_places_(false)
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

  ForceLabelUpdate();
}

void FloatSlider::SetDisplayType(const FloatSlider::DisplayType &type)
{
  display_type_ = type;

  switch (display_type_) {
  case kNormal:
    ClearFormat();
    break;
  case kDecibel:
    SetFormat(tr("%1 dB"));
    break;
  case kPercentage:
    SetFormat(tr("%1%"));
    break;
  }
}

void FloatSlider::SetAutoTrimDecimalPlaces(bool e)
{
  autotrim_decimal_places_ = e;

  ForceLabelUpdate();
}

QString FloatSlider::ValueToString(const QVariant &v)
{
  double val = v.toDouble();

  switch (display_type_) {
  case kNormal:
    // Do nothing, skip to the return string at the end
    break;
  case kDecibel:
    // Convert to decibels and return dB formatted string
    val = QAudio::convertVolume(val, QAudio::LinearVolumeScale, QAudio::DecibelVolumeScale);
    break;
  case kPercentage:
    // Multiply value by 100 for user-friendly percentage
    val *= 100.0;
    break;
  }

  QString s = QString::number(val, 'f', decimal_places_);

  if (autotrim_decimal_places_) {
    while (s.endsWith('0')
           && s.at(s.size() - 2).isDigit()) {
      s = s.left(s.size() - 1);
    }
  }

  return s;
}

QVariant FloatSlider::StringToValue(const QString &s, bool *ok)
{
  switch (display_type_) {
  case kNormal:
    // Do nothing, skip to the return string at the end
    break;
  case kDecibel:
  {
    bool valid;

    // See if we can get a decimal number out of this
    qreal decibels = s.toDouble(&valid);

    if (ok) *ok = valid;

    if (valid) {
      // Convert from decibel scale to linear decimal
      return QAudio::convertVolume(decibels, QAudio::DecibelVolumeScale, QAudio::LinearVolumeScale);
    }

    break;
  }
  case kPercentage:
  {
    bool valid;

    // Try to get double value
    double val = s.toDouble(&valid);

    if (ok) *ok = valid;

    // If we could get it, convert back to a 0.0 - 1.0 value and return
    if (valid) {
      return val * 0.01;
    }

    break;
  }
  }

  // Just try to convert the string to a double
  return s.toDouble(ok);
}

double FloatSlider::AdjustDragDistanceInternal(const double &start, const double &drag)
{
  switch (display_type_) {
  case kNormal:
    // No change here
    break;
  case kDecibel:
  {
    qreal current_db = QAudio::convertVolume(start, QAudio::LinearVolumeScale, QAudio::DecibelVolumeScale);
    current_db += drag;
    qreal adjusted_linear = QAudio::convertVolume(current_db, QAudio::DecibelVolumeScale, QAudio::LinearVolumeScale);

    return adjusted_linear;
  }
  case kPercentage:
    return SliderBase::AdjustDragDistanceInternal(start, drag * 0.01);
  }

  return SliderBase::AdjustDragDistanceInternal(start, drag);
}

void FloatSlider::ConvertValue(QVariant v)
{
  emit ValueChanged(v.toDouble());
}

}
