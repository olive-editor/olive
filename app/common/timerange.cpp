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

#include "timerange.h"

#include <QtMath>
#include <utility>

namespace olive {

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
  bool doesnt_overlap_in = (in_inclusive) ? (a.out() < in()) : (a.out() <= in());

  bool doesnt_overlap_out = (out_inclusive) ? (a.in() > out()) : (a.in() >= out());

  return !doesnt_overlap_in && !doesnt_overlap_out;
}

TimeRange TimeRange::Combined(const TimeRange &a) const
{
  return Combine(a, *this);
}

bool TimeRange::Contains(const TimeRange &compare, bool in_inclusive, bool out_inclusive) const
{
  bool contains_in = (in_inclusive) ? (compare.in() >= in()) : (compare.in() > in());

  bool contains_out = (out_inclusive) ? (compare.out() <= out()) : (compare.out() < out());

  return contains_in && contains_out;
}

bool TimeRange::Contains(const rational &r) const
{
  return r >= in_ && r < out_;
}

TimeRange TimeRange::Combine(const TimeRange &a, const TimeRange &b)
{
  return TimeRange(qMin(a.in(), b.in()),
                   qMax(a.out(), b.out()));
}

TimeRange TimeRange::Intersected(const TimeRange &a) const
{
  return Intersect(a, *this);
}

TimeRange TimeRange::Intersect(const TimeRange &a, const TimeRange &b)
{
  return TimeRange(qMax(a.in(), b.in()),
                   qMin(a.out(), b.out()));
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

std::list<TimeRange> TimeRange::Split(const int &chunk_size) const
{
  std::list<TimeRange> split_ranges;

  int start_time = qFloor(this->in().toDouble() / static_cast<double>(chunk_size)) * chunk_size;
  int end_time = qCeil(this->out().toDouble() / static_cast<double>(chunk_size)) * chunk_size;

  for (int i=start_time; i<end_time; i+=chunk_size) {
    split_ranges.push_back(TimeRange(qMax(this->in(), rational(i)),
                                     qMin(this->out(), rational(i + chunk_size))));
  }

  return split_ranges;
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

void TimeRangeList::insert(TimeRange range_to_add)
{
  // See if list contains this range
  if (contains(range_to_add)) {
    return;
  }

  // Does not contain range, so we'll almost certainly be adding it in some way
  for (int i=0;i<size();i++) {
    const TimeRange& compare = array_.at(i);

    if (compare.OverlapsWith(range_to_add)) {
      range_to_add = TimeRange::Combine(range_to_add, compare);
      array_.removeAt(i);
      i--;
    }
  }

  array_.append(range_to_add);
}

void TimeRangeList::remove(const TimeRange &remove)
{
  int sz = this->size();

  for (int i=0;i<sz;i++) {
    TimeRange& compare = array_[i];

    if (remove.Contains(compare)) {
      // This element is entirely encompassed in this range, remove it
      array_.removeAt(i);
      i--;
      sz--;
    } else if (compare.Contains(remove, false, false)) {
      // The remove range is within this element, only choice is to split the element into two
      TimeRange new_range(remove.out(), compare.out());
      compare.set_out(remove.in());
      insert(new_range);
      break;
    } else if (compare.in() < remove.in() && compare.out() > remove.in()) {
      // This element's out point overlaps the range's in, we'll trim it
      compare.set_out(remove.in());
    } else if (compare.in() < remove.out() && compare.out() > remove.out()) {
      // This element's in point overlaps the range's out, we'll trim it
      compare.set_in(remove.out());
    }
  }
}

bool TimeRangeList::contains(const TimeRange &range, bool in_inclusive, bool out_inclusive) const
{
  for (int i=0;i<size();i++) {
    if (array_.at(i).Contains(range, in_inclusive, out_inclusive)) {
      return true;
    }
  }

  return false;
}

void TimeRangeList::shift(const rational &diff)
{
  for (int i=0; i<array_.size(); i++) {
    array_[i] += diff;
  }
}

void TimeRangeList::trim_in(const rational &diff)
{
  // Re-do list since we want to handle overlaps
  TimeRangeList temp = *this;

  clear();

  foreach (TimeRange r, temp) {
    r.set_in(r.in() + diff);
    insert(r);
  }
}

void TimeRangeList::trim_out(const rational &diff)
{
  // Re-do list since we want to handle overlaps
  TimeRangeList temp = *this;

  clear();

  foreach (TimeRange r, temp) {
    r.set_out(r.out() + diff);
    insert(r);
  }
}

TimeRangeList TimeRangeList::Intersects(const TimeRange &range) const
{
  TimeRangeList intersect_list;

  for (int i=0;i<size();i++) {
    const TimeRange& compare = array_.at(i);

    if (compare.out() <= range.in() || compare.in() >= range.out()) {
      // No intersect
      continue;
    } else {
      // Crop the time range to the range and add it to the list
      TimeRange cropped(qMax(range.in(), compare.in()),
                        qMin(range.out(), compare.out()));

      intersect_list.insert(cropped);
    }
  }

  return intersect_list;
}

uint qHash(const TimeRange &r, uint seed)
{
  return qHash(r.in(), seed) ^ qHash(r.out(), seed);
}

}

QDebug operator<<(QDebug debug, const olive::TimeRange &r)
{
  debug.nospace() << r.in().toDouble() << " - " << r.out().toDouble();
  return debug.space();
}

QDebug operator<<(QDebug debug, const olive::TimeRangeList &r)
{
  debug << r.internal_array();
  return debug.space();
}
