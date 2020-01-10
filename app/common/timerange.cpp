#include "timerange.h"

#include <utility>

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

bool TimeRange::OverlapsWith(const TimeRange &a) const
{
  return Overlap(a, *this);
}

TimeRange TimeRange::CombineWith(const TimeRange &a) const
{
  return Combine(a, *this);
}

bool TimeRange::Contains(const TimeRange &compare, bool inout_inclusive) const
{
  if (inout_inclusive) {
    return (compare.in() >= in() && compare.out() <= out());
  } else {
    return (compare.in() > in() && compare.out() < out());
  }
}

bool TimeRange::Overlap(const TimeRange &a, const TimeRange &b)
{
  return !(a.out() < b.in() || a.in() > b.out());
}

TimeRange TimeRange::Combine(const TimeRange &a, const TimeRange &b)
{
  return TimeRange(qMin(a.in(), b.in()),
                   qMax(a.out(), b.out()));
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

uint qHash(const TimeRange &r, uint seed)
{
  return qHash(r.in(), seed) ^ qHash(r.out(), seed);
}

void TimeRangeList::InsertTimeRange(const TimeRange &range)
{
  for (int i=0;i<size();i++) {
    const TimeRange& compare = at(i);

    if (TimeRange::Overlap(range, compare)) {
      replace(i, TimeRange::Combine(range, compare));
      return;
    }
  }

  append(range);
}

void TimeRangeList::RemoveTimeRange(const TimeRange &range)
{
  for (int i=0;i<size();i++) {
    const TimeRange& compare = at(i);

    if (range.Contains(compare)) {
      // This element is entirely encompassed in this range, remove it
      removeAt(i);
      i--;
    } else if (compare.Contains(range, false)) {
      // The remove range is within this element, only choice is to split the element into two
      TimeRange first(compare.in(), range.in());
      TimeRange last(range.out(), compare.out());

      replace(i, first);
      append(last);
    } else if (compare.in() < range.in() && compare.out() > range.in()) {
      // This element's out point overlaps the range's in, we'll trim it
      TimeRange trimmed = compare;
      trimmed.set_out(range.in());
      replace(i, trimmed);
    } else if (compare.in() < range.out() && compare.out() > range.out()) {
      // This element's in point overlaps the range's out, we'll trim it
      TimeRange trimmed = compare;
      trimmed.set_in(range.out());
      replace(i, trimmed);
    }
  }
}

bool TimeRangeList::ContainsTimeRange(const TimeRange &range) const
{
  for (int i=0;i<size();i++) {
    if (at(i).Contains(range)) {
      return true;
    }
  }

  return false;
}

TimeRangeList TimeRangeList::Intersects(const TimeRange &range)
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
