#include "timetargetobject.h"

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

rational TimeTargetObject::GetAdjustedTime(Node* from, Node* to, const rational &r, NodeParam::Type direction) const
{
  if (!from || !to) {
    return r;
  }

  return GetAdjustedTime(from, to, TimeRange(r, r), direction).in();
}

TimeRange TimeTargetObject::GetAdjustedTime(Node* from, Node* to, const TimeRange &r, NodeParam::Type direction) const
{
  if (!from || !to) {
    return r;
  }

  QList<TimeRange> adjusted = from->TransformTimeTo(r, to, direction);

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
