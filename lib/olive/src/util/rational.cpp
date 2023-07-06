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

#include "util/rational.h"

#include <QStringList>
#include <math.h>

namespace olive {

const rational rational::NaN = rational(0, 0);

rational rational::fromDouble(const double &flt, bool* ok)
{
  if (isnan(flt)) {
    // Return NaN rational
    if (ok) *ok = false;
    return NaN;
  }

  // Use FFmpeg function for the time being
  AVRational r = av_d2q(flt, INT_MAX);

  if (r.den == 0) {
    // If den == 0, we were unable to convert to a rational
    if (ok) {
      *ok = false;
    }
  } else {
    // Otherwise, assume we received a real rational
    if (ok) {
      *ok = true;
    }
  }

  return r;
}

rational rational::fromString(const QString &str, bool* ok)
{
  QStringList elements = str.split('/');

  bool ok2;
  switch (elements.size()) {
  case 1:
  {
    int num = elements.front().toInt(&ok2);
    if (ok2) {
      if (ok) *ok = true;
      return num;
    }
  }
  case 2:
  {
    int num = elements.at(0).toInt(&ok2);
    if (ok2) {
      int den = elements.at(1).toInt(&ok2);
      if (ok2) {
        if (ok) *ok = true;
        return rational(num, den);
      }
    }
  }
  default:
    break;
  }

  // Returns NaN with ok set to false
  if (ok) {
    *ok = false;
  }
  return NaN;
}

//Function: convert to double

double rational::toDouble() const
{
  if (r_.den != 0) {
    return av_q2d(r_);
  } else {
    return std::numeric_limits<double>::quiet_NaN();
  }
}

AVRational rational::toAVRational() const
{
  return r_;
}

#ifdef USE_OTIO
opentime::RationalTime rational::toRationalTime(double framerate) const
{
  // Is this the best way of doing this?
  // Olive can store rationals as 0/0 which causes errors in OTIO
  opentime::RationalTime time = opentime::RationalTime(r_.num, r_.den == 0 ? 1 : r_.den);
  return time.rescaled_to(framerate);
}
#endif

rational rational::flipped() const
{
  rational r = *this;
  r.flip();
  return r;
}

void rational::flip()
{
  if (!isNull()) {
    std::swap(r_.den, r_.num);
    fix_signs();
  }
}

QString rational::toString() const
{
  return QStringLiteral("%1/%2").arg(QString::number(r_.num), QString::number(r_.den));
}

void rational::fix_signs()
{
  if (r_.den < 0) {
    // Normalize so that denominator is always positive
    r_.den = -r_.den;
    r_.num = -r_.num;
  } else if (r_.den == 0) {
    // Normalize to 0/0 (aka NaN) if denominator is zero
    r_.num = 0;
  } else if (r_.num == 0) {
    // Normalize to 0/1 if numerator is zero
    r_.den = 1;
  }
}

void rational::reduce()
{
  av_reduce(&r_.num, &r_.den, r_.num, r_.den, INT_MAX);
}

//Assignment Operators

const rational& rational::operator=(const rational &rhs)
{
  r_ = rhs.r_;
  return *this;
}

const rational& rational::operator+=(const rational &rhs)
{
  if(*this == RATIONAL_MIN || *this == RATIONAL_MAX || rhs == RATIONAL_MIN || rhs == RATIONAL_MAX) {
    *this = NaN;
  } else if (!isNaN()) {
    if (rhs.isNaN()) {
      *this = NaN;
    } else {
      r_ = av_add_q(r_, rhs.r_);
      fix_signs();
    }
  }

  return *this;
}

const rational& rational::operator-=(const rational &rhs)
{
  if (*this == RATIONAL_MIN || *this == RATIONAL_MAX || rhs == RATIONAL_MIN || rhs == RATIONAL_MAX) {
    *this = NaN;
  } else if (!isNaN()) {
    if (rhs.isNaN()) {
      *this = NaN;
    } else {
      r_ = av_sub_q(r_, rhs.r_);
      fix_signs();
    }
  }

  return *this;
}

const rational& rational::operator*=(const rational &rhs)
{
  if (*this == RATIONAL_MIN || *this == RATIONAL_MAX || rhs == RATIONAL_MIN || rhs == RATIONAL_MAX) {
    *this = NaN;
  } else if (!isNaN()) {
    if (rhs.isNaN()) {
      *this = NaN;
    } else {
      r_ = av_mul_q(r_, rhs.r_);
      fix_signs();
    }
  }

  return *this;
}

const rational& rational::operator/=(const rational &rhs)
{
  if (*this == RATIONAL_MIN || *this == RATIONAL_MAX || rhs == RATIONAL_MIN || rhs == RATIONAL_MAX) {
    *this = NaN;
  } else if (!isNaN()) {
    if (rhs.isNaN()) {
      *this = NaN;
    } else {
      r_ = av_div_q(r_, rhs.r_);
      fix_signs();
    }
  }

  return *this;
}

//Binary math operators

rational rational::operator+(const rational &rhs) const
{
  rational answer(*this);
  answer += rhs;
  return answer;
}

rational rational::operator-(const rational &rhs) const
{
  rational answer(*this);
  answer -= rhs;
  return answer;
}

rational rational::operator/(const rational &rhs) const
{
  rational answer(*this);
  answer /= rhs;
  return answer;
}

rational rational::operator*(const rational &rhs) const
{
  rational answer(*this);
  answer *= rhs;
  return answer;
}

//Relational and equality operators

bool rational::operator<(const rational &rhs) const
{
  return av_cmp_q(r_, rhs.r_) == -1;
}

bool rational::operator<=(const rational &rhs) const
{
  int cmp = av_cmp_q(r_, rhs.r_);
  return cmp == 0 || cmp == -1;
}

bool rational::operator>(const rational &rhs) const
{
  return av_cmp_q(r_, rhs.r_) == 1;
}

bool rational::operator>=(const rational &rhs) const
{
  int cmp = av_cmp_q(r_, rhs.r_);
  return cmp == 0 || cmp == 1;
}

bool rational::operator==(const rational &rhs) const
{
  return av_cmp_q(r_, rhs.r_) == 0;
}

bool rational::operator!=(const rational &rhs) const
{
  return !(*this == rhs);
}

}
