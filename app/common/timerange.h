#ifndef TIMERANGE_H
#define TIMERANGE_H

#include "rational.h"

class TimeRange {
public:
  TimeRange();
  TimeRange(const rational& in, const rational& out);

  const rational& in() const;
  const rational& out() const;

  void set_in(const rational& in);
  void set_out(const rational& out);
  void set_range(const rational& in, const rational& out);

private:
  void normalize();

  rational in_;
  rational out_;
};

#endif // TIMERANGE_H
