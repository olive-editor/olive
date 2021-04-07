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

#include "rationalslider.h"

#include "common/timecodefunctions.h"
#include "core.h"

namespace olive {

RationalSlider::RationalSlider(QWidget *parent) :
  SliderBase(SliderBase::kRational, parent),
  display_type_(kTimecode),
  decimal_places_(2),
  autotrim_decimal_places_(false),
  lock_display_type_(false)
{
  connect(this, SIGNAL(ValueChanged(QVariant)), this, SLOT(ConvertValue(QVariant)));
  connect(this, &SliderBase::changeRationalDisplayType, this, &RationalSlider::changeDisplayType);
  connect(Core::instance(), &Core::TimecodeDisplayChanged, this, &RationalSlider::ChangeTimecodeDisplayType);

  SetDisplayType(display_type_);

  SetValue(rational(0, 0));
}

rational RationalSlider::GetValue()
{
  return Value().value<rational>();
}

void RationalSlider::SetValue(const rational &d)
{
  SliderBase::SetValue(QVariant::fromValue(d));
}

void RationalSlider::SetDefaultValue(const rational &r)
{
  SliderBase::SetDefaultValue(QVariant::fromValue(r));
}

void RationalSlider::SetDefaultValue(const QVariant &v) {
  rational r = v.value<rational>();
  SetDefaultValue(r);
}

void RationalSlider::SetMinimum(const rational &d)
{
  SetMinimumInternal(QVariant::fromValue(d));
}

void RationalSlider::SetMaximum(const rational &d)
{
  SetMaximumInternal(QVariant::fromValue(d));
}

void RationalSlider::SetDecimalPlaces(int i)
{
  decimal_places_ = i;

  ForceLabelUpdate();
}

void RationalSlider::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;

  // Refresh label since we have a new timebase to generate a timecode with
  UpdateLabel(Value());
}

void RationalSlider::SetAutoTrimDecimalPlaces(bool e) {
  autotrim_decimal_places_ = e;

  ForceLabelUpdate();
}

void RationalSlider::SetDisplayType(const RationalSlider::DisplayType &type)
{
  display_type_ = type;

  switch (display_type_) {
    case kTimecode:
    case kRational:
      ClearFormat();
      break;
    case kTimestamp:
      SetFormat("%1 Frames");
      break;
    case kFloat:
      SetFormat("%1 s");
      break;
  }
}

void RationalSlider::SetLockDisplayType(bool e)
{
  lock_display_type_ = e;
}

bool RationalSlider::LockDisplayType()
{
  return lock_display_type_;
}

QString RationalSlider::ValueToString(const QVariant &v)
{
  double time = v.value<rational>().toDouble() + GetOffset().value<rational>().toDouble();

  switch (display_type_) {
    case kTimecode:
      return Timecode::time_to_timecode(v.value<rational>(), timebase_, Core::instance()->GetTimecodeDisplay());
    case kTimestamp:
      return QString::number(Timecode::time_to_timestamp(time, timebase_));
    case kRational:
      // Might we want to call reduce() on r here?
      return v.value<rational>().toString();
    case kFloat:
    {
      QString s = QString::number(time, 'f', decimal_places_);

      if (autotrim_decimal_places_) {
        while (s.endsWith('0') && s.at(s.size() - 2).isDigit()) {
          s = s.left(s.size() - 1);
        }
      }
      return s;
    }
  }
  return v.toString();
}

QVariant RationalSlider::StringToValue(const QString &s, bool *ok)
{
  rational r;
  *ok = false;

  switch (display_type_) {
    case kTimecode:
    {
      int t = Timecode::timecode_to_timestamp(s, timebase_, Core::instance()->GetTimecodeDisplay(), ok);
      r = rational(t, timebase_.denominator());
      break;
    }
    case kTimestamp:
      r = rational(s.toInt(ok), timebase_.denominator());
      break;
    case kRational:
      r = rational::fromString(s);
      if (!r.isNull()) {
        *ok = true;
      }
      break;
    case kFloat:
      r = rational::fromDouble(s.toDouble(ok));
      if (!r.isNull()) {
        *ok = true;
      }
      break;
  }

  return QVariant::fromValue(r - GetOffset().value<rational>());
}

double RationalSlider::AdjustDragDistanceInternal(const double &start, const double &drag)
{
  // Assume we want smallest increment to be timebase or 1 frame
  return start + drag*timebase_.toDouble();
}

void RationalSlider::ConvertValue(QVariant v)
{
  emit ValueChanged(v.value<rational>());
}

void RationalSlider::changeDisplayType()
{
  if (!LockDisplayType()) {
    // Loop through the display types
    SetDisplayType(static_cast<RationalSlider::DisplayType>(((int)(display_type_) + 1) % 4));
    ForceLabelUpdate();
  }
}

void RationalSlider::ChangeTimecodeDisplayType()
{
  ForceLabelUpdate();
}

}
