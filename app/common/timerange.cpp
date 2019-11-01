#include "timerange.h"

TimeRange::TimeRange()
{
}

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

bool TimeRange::operator<(const TimeRange &r) const
{
  return length() < r.length();
}

bool TimeRange::operator<=(const TimeRange &r) const
{
  return length() <= r.length();
}

bool TimeRange::operator>(const TimeRange &r) const
{
  return length() > r.length();
}

bool TimeRange::operator>=(const TimeRange &r) const
{
  return length() >= r.length();
}

bool TimeRange::operator==(const TimeRange &r) const
{
  return length() == r.length();
}

bool TimeRange::operator!=(const TimeRange &r) const
{
  return length() != r.length();
}

void TimeRange::normalize()
{
  // If `out` is earlier than `in`, swap them
  if (out_ < in_) {
    rational temp = in_;
    in_ = out_;
    out_ = temp;
  }

  // Calculate length
  length_ = out_ - in_;
}
