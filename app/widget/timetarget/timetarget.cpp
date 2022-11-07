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

#include "timetarget.h"

namespace olive {

TimeTargetObject::TimeTargetObject() :
  time_target_(nullptr),
  path_index_(0)
{
}

ViewerOutput *TimeTargetObject::GetTimeTarget() const
{
  return time_target_;
}

void TimeTargetObject::SetTimeTarget(ViewerOutput *target)
{
  if (time_target_) {
    TimeTargetDisconnectEvent(time_target_);
  }

  time_target_ = target;
  TimeTargetChangedEvent(time_target_);

  if (time_target_) {
    TimeTargetConnectEvent(time_target_);
  }
}

void TimeTargetObject::SetPathIndex(int index)
{
  path_index_ = index;
}

rational TimeTargetObject::GetAdjustedTime(Node* from, Node* to, const rational &r, Node::TransformTimeDirection dir) const
{
  if (!from || !to) {
    return r;
  }

  return GetAdjustedTime(from, to, TimeRange(r, r), dir).in();
}

TimeRange TimeTargetObject::GetAdjustedTime(Node* from, Node* to, const TimeRange &r, Node::TransformTimeDirection dir) const
{
  if (!from || !to) {
    return r;
  }

  return from->TransformTimeTo(r, to, dir, path_index_);
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
