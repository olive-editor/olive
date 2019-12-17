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

#include <QAudio>

FloatSlider::FloatSlider(QWidget *parent) :
  SliderBase(kFloat, parent),
  display_type_(kNormal)
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

void FloatSlider::SetDisplayType(const FloatSlider::DisplayType &type)
{
  display_type_ = type;

  UpdateLabel(Value());
}

QString FloatSlider::ValueToString(const QVariant &v)
{
  double val = v.toDouble();

  switch (display_type_) {
  case kNormal:
    // Do nothing, skip to the return string at the end
    break;
  case kDecibel:
  {
    // Convert to decibels and return dB formatted string
    qreal decibels = QAudio::convertVolume(val, QAudio::LinearVolumeScale, QAudio::DecibelVolumeScale);
    return QStringLiteral("%1 dB").arg(QString::number(decibels, 'f', decimal_places_));
  }
  case kPercentage:
    // Multiply value by 100 for user-friendly percentage
    val *= 100.0;
    return QStringLiteral("%1%").arg(QString::number(val, 'f', decimal_places_));
  }

  return QString::number(val, 'f', decimal_places_);
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

    // Remove any instance of "dB" in the string
    QString decibel_number = s;
    decibel_number.replace("dB", "", Qt::CaseInsensitive);

    // See if we can get a decimal number out of this
    qreal decibels = decibel_number.toDouble(&valid);

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

    QString percent_number = s;
    percent_number.replace("%", "", Qt::CaseInsensitive);

    // Try to get double value
    double val = percent_number.toDouble(&valid);

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

    return adjusted_linear - start;
  }
  case kPercentage:
    return drag * 0.01;
  }

  return SliderBase::AdjustDragDistanceInternal(start, drag);
}

void FloatSlider::ConvertValue(QVariant v)
{
  emit ValueChanged(v.toDouble());
}
