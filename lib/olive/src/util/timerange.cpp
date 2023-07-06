/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#include "util/timerange.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "util/timecodefunctions.h"

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
  return TimeRange(std::min(a.in(), b.in()),
                   std::max(a.out(), b.out()));
}

TimeRange TimeRange::Intersected(const TimeRange &a) const
{
  return Intersect(a, *this);
}

TimeRange TimeRange::Intersect(const TimeRange &a, const TimeRange &b)
{
  return TimeRange(std::max(a.in(), b.in()),
                   std::min(a.out(), b.out()));
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

  int start_time = std::floor(this->in().toDouble() / static_cast<double>(chunk_size)) * chunk_size;
  int end_time = std::ceil(this->out().toDouble() / static_cast<double>(chunk_size)) * chunk_size;

  for (int i=start_time; i<end_time; i+=chunk_size) {
    split_ranges.push_back(TimeRange(std::max(this->in(), rational(i)),
                                     std::min(this->out(), rational(i + chunk_size))));
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
  if (out_ == RATIONAL_MIN || out_ == RATIONAL_MAX || in_ == RATIONAL_MIN || in_ == RATIONAL_MAX) {
    length_ = rational::NaN;
  } else {
    length_ = out_ - in_;
  }
}

void TimeRangeList::insert(const TimeRangeList &list_to_add)
{
  for (auto it=list_to_add.cbegin(); it!=list_to_add.cend(); it++) {
    insert(*it);
  }
}

void TimeRangeList::insert(TimeRange range_to_add)
{
  // See if list contains this range
  if (contains(range_to_add)) {
    return;
  }

  // Does not contain range, so we'll almost certainly be adding it in some way
  for (auto it = array_.begin(); it != array_.end(); ) {
    const TimeRange& compare = *it;

    if (compare.OverlapsWith(range_to_add)) {
      range_to_add = TimeRange::Combine(range_to_add, compare);
      it = array_.erase(it);
    } else {
      it++;
    }
  }

  array_.push_back(range_to_add);
}

void TimeRangeList::remove(const TimeRange &remove)
{
  util_remove(&array_, remove);
}

void TimeRangeList::remove(const TimeRangeList &list)
{
  for (const TimeRange &r : list) {
    remove(r);
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

  for (auto it = temp.array_.begin(); it != temp.array_.end(); it++) {
    TimeRange &r = *it;
    r.set_in(r.in() + diff);
    insert(r);
  }
}

void TimeRangeList::trim_out(const rational &diff)
{
  // Re-do list since we want to handle overlaps
  TimeRangeList temp = *this;

  clear();

  for (auto it = temp.array_.begin(); it != temp.array_.end(); it++) {
    TimeRange &r = *it;
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
      TimeRange cropped(std::max(range.in(), compare.in()),
                        std::min(range.out(), compare.out()));

      intersect_list.insert(cropped);
    }
  }

  return intersect_list;
}

TimeRangeListFrameIterator::TimeRangeListFrameIterator() :
  TimeRangeListFrameIterator(TimeRangeList(), rational::NaN)
{
}

TimeRangeListFrameIterator::TimeRangeListFrameIterator(const TimeRangeList &list, const rational &timebase) :
  list_(list),
  timebase_(timebase),
  range_index_(-1),
  size_(-1),
  frame_index_(0),
  custom_range_(false)
{
  if (!list_.isEmpty() && timebase_.isNull()) {
    std::cerr << "TimeRangeListFrameIterator created with null timebase but non-empty list, this will likely lead to infinite loops" << std::endl;
  }

  UpdateIndexIfNecessary();
}

rational TimeRangeListFrameIterator::Snap(const rational &r) const
{
  return Timecode::snap_time_to_timebase(r, timebase_, Timecode::kFloor);
}

bool TimeRangeListFrameIterator::GetNext(rational *out)
{
  if (!HasNext()) {
    return false;
  }

  // Output current value
  *out = current_;

  // Determine next value by adding timebase
  current_ += timebase_;

  // If this time is outside the current range, jump to the next one
  UpdateIndexIfNecessary();

  // Increment frame index
  frame_index_++;

  return true;
}

bool TimeRangeListFrameIterator::HasNext() const
{
  return range_index_ < list_.size();
}

int TimeRangeListFrameIterator::size()
{
  if (size_ == -1) {
    // Size isn't calculated automatically for optimization, so we'll calculate it now
    size_ = 0;

    for (const TimeRange &range : list_) {
      rational start = Snap(range.in());
      rational end = Timecode::snap_time_to_timebase(range.out(), timebase_, Timecode::kFloor);

      if (end == range.out()) {
        end -= timebase_;
      }

      int64_t start_ts = Timecode::time_to_timestamp(start, timebase_);
      int64_t end_ts = Timecode::time_to_timestamp(end, timebase_);

      size_ += 1 + (end_ts - start_ts);
    }
  }

  return size_;
}

void TimeRangeListFrameIterator::UpdateIndexIfNecessary()
{
  while (range_index_ < list_.size() && (range_index_ == -1 || current_ >= list_.at(range_index_).out())) {
    range_index_++;

    if (range_index_ < list_.size()) {
      current_ = Snap(list_.at(range_index_).in());
    }
  }
}

}
