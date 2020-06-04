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

#include "timerange.h"

#include <utility>

OLIVE_NAMESPACE_ENTER

TimeRange::TimeRange(const rational &in, const rational &out) :
  in_(in),
  out_(out)
{
  normalize();
}

const rational &TimeRange::in() const
{
  return in_;
}

const rational &TimeRange::out() const
{
  return out_;
}

const rational &TimeRange::length() const
{
  return length_;
}

void TimeRange::set_in(const rational &in)
{
  in_ = in;
  normalize();
}

void TimeRange::set_out(const rational &out)
{
  out_ = out;
  normalize();
}

void TimeRange::set_range(const rational &in, const rational &out)
{
  in_ = in;
  out_ = out;
  normalize();
}

bool TimeRange::operator==(const TimeRange &r) const
{
  return in() == r.in() && out() == r.out();
}

bool TimeRange::operator!=(const TimeRange &r) const
{
  return in() != r.in() || out() != r.out();
}

bool TimeRange::OverlapsWith(const TimeRange &a, bool in_inclusive, bool out_inclusive) const
{
  bool overlaps_in = (in_inclusive) ? (a.out() < in()) : (a.out() <= in());

  bool overlaps_out = (out_inclusive) ? (a.in() > out()) : (a.in() >= out());

  return !(overlaps_in || overlaps_out);
}

TimeRange TimeRange::CombineWith(const TimeRange &a) const
{
  return Combine(a, *this);
}

bool TimeRange::Contains(const TimeRange &compare, bool in_inclusive, bool out_inclusive) const
{
  bool contains_in = (in_inclusive) ? (compare.in() >= in()) : (compare.in() > in());

  bool contains_out = (out_inclusive) ? (compare.out() <= out()) : (compare.out() < out());

  return contains_in && contains_out;
}

TimeRange TimeRange::Combine(const TimeRange &a, const TimeRange &b)
{
  return TimeRange(qMin(a.in(), b.in()),
                   qMax(a.out(), b.out()));
}

TimeRange TimeRange::operator+(const rational &rhs) const
{
  TimeRange answer(*this);
  answer += rhs;
  return answer;
}

TimeRange TimeRange::operator-(const rational &rhs) const
{
  TimeRange answer(*this);
  answer -= rhs;
  return answer;
}

const TimeRange &TimeRange::operator+=(const rational &rhs)
{
  set_range(in_ + rhs, out_ + rhs);

  return *this;
}

const TimeRange &TimeRange::operator-=(const rational &rhs)
{
  set_range(in_ - rhs, out_ - rhs);

  return *this;
}

void TimeRange::normalize()
{
  // If `out` is earlier than `in`, swap them
  if (out_ < in_)
  {
    std::swap(out_, in_);
  }

  // Calculate length
  length_ = out_ - in_;
}

void TimeRangeList::InsertTimeRange(const TimeRange &range)
{
  for (int i=0;i<size();i++) {
    const TimeRange& compare = at(i);

    if (compare == range) {
      return;
    } else if (range.OverlapsWith(compare)) {
      replace(i, TimeRange::Combine(range, compare));
      return;
    }
  }

  append(range);
}

void TimeRangeList::RemoveTimeRange(const TimeRange &range)
{
  RemoveTimeRange(this, range);
}

void TimeRangeList::RemoveTimeRange(QList<TimeRange> *list, const TimeRange &remove)
{
  for (int i=0;i<list->size();i++) {
    TimeRange& compare = (*list)[i];

    if (remove.Contains(compare)) {
      // This element is entirely encompassed in this range, remove it
      list->removeAt(i);
      i--;
    } else if (compare.Contains(remove, false, false)) {
      // The remove range is within this element, only choice is to split the element into two
      list->append(TimeRange(remove.out(), compare.out()));
      compare.set_out(remove.in());
    } else if (compare.in() < remove.in() && compare.out() > remove.in()) {
      // This element's out point overlaps the range's in, we'll trim it
      compare.set_out(remove.in());
    } else if (compare.in() < remove.out() && compare.out() > remove.out()) {
      // This element's in point overlaps the range's out, we'll trim it
      compare.set_in(remove.out());
    }
  }
}

bool TimeRangeList::ContainsTimeRange(const TimeRange &range, bool in_inclusive, bool out_inclusive) const
{
  for (int i=0;i<size();i++) {
    if (at(i).Contains(range, in_inclusive, out_inclusive)) {
      return true;
    }
  }

  return false;
}

TimeRangeList TimeRangeList::Intersects(const TimeRange &range) const
{
  TimeRangeList intersect_list;

  for (int i=0;i<size();i++) {
    const TimeRange& compare = at(i);

    if (compare.out() <= range.in() || compare.in() >= range.out()) {
      // No intersect
      continue;
    } else {
      // Crop the time range to the range and add it to the list
      TimeRange cropped(qMax(range.in(), compare.in()),
                        qMin(range.out(), compare.out()));

      intersect_list.append(cropped);
    }
  }

  return intersect_list;
}

void TimeRangeList::PrintTimeList()
{
  qDebug() << "TimeRangeList now contains:";

  for (int i=0;i<size();i++) {
    qDebug() << "  " << at(i);
  }
}

uint qHash(const TimeRange &r, uint seed)
{
  return qHash(r.in(), seed) ^ qHash(r.out(), seed);
}

OLIVE_NAMESPACE_EXIT

QDebug operator<<(QDebug debug, const OLIVE_NAMESPACE::TimeRange &r)
{
  debug.nospace() << r.in().toDouble() << " - " << r.out().toDouble();
  return debug.space();
}
