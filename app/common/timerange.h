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

#ifndef TIMERANGE_H
#define TIMERANGE_H

#include "rational.h"

namespace olive {

class TimeRange {
public:
  TimeRange() = default;
  TimeRange(const rational& in, const rational& out);

  const rational& in() const;
  const rational& out() const;
  const rational& length() const;

  void set_in(const rational& in);
  void set_out(const rational& out);
  void set_range(const rational& in, const rational& out);

  bool operator==(const TimeRange& r) const;
  bool operator!=(const TimeRange& r) const;

  bool OverlapsWith(const TimeRange& a, bool in_inclusive = true, bool out_inclusive = true) const;
  bool Contains(const TimeRange& a, bool in_inclusive = true, bool out_inclusive = true) const;
  bool Contains(const rational& r) const;

  TimeRange Combined(const TimeRange& a) const;
  static TimeRange Combine(const TimeRange &a, const TimeRange &b);
  TimeRange Intersected(const TimeRange& a) const;
  static TimeRange Intersect(const TimeRange &a, const TimeRange &b);

  TimeRange operator+(const rational& rhs) const;
  TimeRange operator-(const rational& rhs) const;

  const TimeRange& operator+=(const rational &rhs);
  const TimeRange& operator-=(const rational &rhs);

  std::list<TimeRange> Split(const int &chunk_size) const;

private:
  void normalize();

  rational in_;
  rational out_;
  rational length_;

};

class TimeRangeList {
public:
  TimeRangeList() = default;

  TimeRangeList(std::initializer_list<TimeRange> r) :
    array_(r)
  {
  }

  void insert(TimeRange range_to_add);

  void remove(const TimeRange& remove);

  bool contains(const TimeRange& range, bool in_inclusive = true, bool out_inclusive = true) const;

  bool isEmpty() const
  {
    return array_.isEmpty();
  }

  void clear()
  {
    array_.clear();
  }

  int size() const
  {
    return array_.size();
  }

  void shift(const rational& diff);

  void trim_in(const rational& diff);

  void trim_out(const rational& diff);

  TimeRangeList Intersects(const TimeRange& range) const;

  using const_iterator = QVector<TimeRange>::const_iterator;

  const_iterator begin() const
  {
    return array_.constBegin();
  }

  const_iterator end() const
  {
    return array_.constEnd();
  }

  const TimeRange& first() const
  {
    return array_.first();
  }

  const TimeRange& last() const
  {
    return array_.last();
  }

  const QVector<TimeRange>& internal_array() const
  {
    return array_;
  }

private:
  QVector<TimeRange> array_;

};

uint qHash(const TimeRange& r, uint seed);

}

QDebug operator<<(QDebug debug, const olive::TimeRange& r);
QDebug operator<<(QDebug debug, const olive::TimeRangeList& r);

Q_DECLARE_METATYPE(olive::TimeRange)

#endif // TIMERANGE_H
