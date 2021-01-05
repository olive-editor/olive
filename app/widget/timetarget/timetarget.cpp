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

#include "timetarget.h"

namespace olive {

TimeTargetObject::TimeTargetObject() :
  time_target_(nullptr),
  path_index_(0)
{
}

Node *TimeTargetObject::GetTimeTarget() const
{
  return time_target_;
}

void TimeTargetObject::SetTimeTarget(Node *target)
{
  time_target_ = target;

  TimeTargetChangedEvent(time_target_);
}

void TimeTargetObject::SetPathIndex(int index)
{
  path_index_ = index;
}

rational TimeTargetObject::GetAdjustedTime(Node* from, Node* to, const rational &r, bool input_direction) const
{
  if (!from || !to) {
    return r;
  }

  return GetAdjustedTime(from, to, TimeRange(r, r), input_direction).in();
}

TimeRange TimeTargetObject::GetAdjustedTime(Node* from, Node* to, const TimeRange &r, bool input_direction) const
{
  if (!from || !to) {
    return r;
  }

  QVector<TimeRange> adjusted = from->TransformTimeTo(r, to, input_direction);

  if (adjusted.isEmpty()) {
    return r;
  }

  return adjusted.at(path_index_);
}

/*int TimeTargetObject::GetNumberOfPathAdjustments(Node* from, NodeParam::Type direction) const
{
  if (!time_target_) {
    return 0;
  }

  QList<TimeRange> adjusted = from->TransformTimeTo(TimeRange(), time_target_, direction);

  return adjusted.size();
}*/

}
