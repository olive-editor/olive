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

#include "timeslider.h"

#include "common/timecodefunctions.h"
#include "core.h"

namespace olive {

TimeSlider::TimeSlider(QWidget *parent) :
  IntegerSlider(parent)
{
  SetMinimum(0);
  SetValue(0);

  connect(Core::instance(), &Core::TimecodeDisplayChanged, this, &TimeSlider::TimecodeDisplayChanged);
}

void TimeSlider::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;

  // Refresh label since we have a new timebase to generate a timecode with
  UpdateLabel(Value());
}

QString TimeSlider::ValueToString(const QVariant &v)
{
  if (timebase_.isNull()) {
    // We can't generate a timecode without a timebase, so we just return the number
    return IntegerSlider::ValueToString(v);
  }

  return Timecode::timestamp_to_timecode(v.toLongLong() + GetOffset().toLongLong(),
                                         timebase_,
                                         Core::instance()->GetTimecodeDisplay());
}

QVariant TimeSlider::StringToValue(const QString &s, bool *ok)
{
  return QVariant::fromValue(Timecode::timecode_to_timestamp(s, timebase_, Core::instance()->GetTimecodeDisplay(), ok) - GetOffset().toLongLong());
}

void TimeSlider::TimecodeDisplayChanged()
{
  UpdateLabel(Value());
}

}
