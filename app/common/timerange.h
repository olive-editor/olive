#ifndef TIMERANGE_H
#define TIMERANGE_H

#include "rational.h"

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

  bool operator<(const TimeRange &r) const;
  bool operator>(const TimeRange &r) const;

private:
  void normalize();

  rational in_;
  rational out_;
  rational length_;
};

#endif // TIMERANGE_H
