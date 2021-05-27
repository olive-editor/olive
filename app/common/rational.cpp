//Copyright 2015 Adam Quintero
//This program is distributed under the terms of the GNU General Public License.

#include "rational.h"

namespace olive {

const rational rational::NaN = rational(0, 0);

rational rational::fromDouble(const double &flt, bool* ok)
{
  if (qIsNaN(flt)) {
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

  switch (elements.size()) {
  case 1:
    return rational(elements.first().toLongLong(ok));
  case 2:
    return rational(elements.at(0).toLongLong(ok), elements.at(1).toLongLong(ok));
  default:
    // Returns NaN with ok set to false
    if (ok) {
      *ok = false;
    }
    return NaN;
  }
}

//Function: ensures denom >= 0

void rational::fix_signs()
{
  // Normalize so that denominator is always positive and only numerator is positive
  if (denom_ < 0) {
    denom_ = -denom_;
    numer_ = -numer_;
  } else if (denom_ == intType(0)) {
    // Normalize to 0/0 (aka NaN) if denominator is zero
    numer_ = intType(0);
  } else if (numer_ == intType(0)) {
    // Normalize to 0/1 if numerator is zero
    denom_ = intType(1);
  }
}

//Function: ensures lowest form

void rational::reduce()
{
  if (!isNull()) {
    // Euclidean often fails if numbers are negative, we abs it and re-neg it later if necessary
    bool neg = numer_ < 0;

    numer_ = qAbs(numer_);

    intType d = gcd(numer_, denom_);

    if (d > 1) {
      numer_ /= d;
      denom_ /= d;
    }

    if (neg) {
      numer_ = -numer_;
    }
  }
}

//Function: finds greatest common denominator

intType rational::gcd(const intType &x, const intType &y)
{
  if (y == 0) {
    return x;
  } else {
    return gcd(y, x % y);
  }
}

//Function: convert to double

double rational::toDouble() const
{
  if (denom_ != 0) {
    return static_cast<double>(numer_) / static_cast<double>(denom_);
  } else {
    return qSNaN();
  }
}

AVRational rational::toAVRational() const
{
  AVRational r;

  r.num = static_cast<int>(numer_);
  r.den = static_cast<int>(denom_);

  return r;
}

#ifdef USE_OTIO
opentime::RationalTime rational::toRationalTime(double framerate) const
{
  // Is this the best way of doing this?
  // Olive can store rationals as 0/0 which causes errors in OTIO
  opentime::RationalTime time = opentime::RationalTime(numer_, denom_ == 0 ? 1 : denom_);
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
    std::swap(denom_, numer_);
  }
}

bool rational::isNull() const
{
  return numerator() == 0;
}

bool rational::isNaN() const
{
  return denominator() == 0;
}

const intType &rational::numerator() const
{
  return numer_;
}

const intType &rational::denominator() const
{
  return denom_;
}

QString rational::toString() const
{
  return QStringLiteral("%1/%2").arg(QString::number(numer_), QString::number(denom_));
}

//Assignment Operators

const rational& rational::operator=(const rational &rhs)
{
  if (this != &rhs) {
    numer_ = rhs.numer_;
    denom_ = rhs.denom_;
  }

  return *this;
}

const rational& rational::operator+=(const rational &rhs)
{
  if (!isNaN()) {
    if (rhs.isNaN()) {
      // Set to NaN
      denom_ = 0;
      fix_signs();
    } else if (!rhs.isNull()) {
      if (isNull()) {
        numer_ = rhs.numer_;
        denom_ = rhs.denom_;
      } else {
        numer_ = (numer_ * rhs.denom_) + (rhs.numer_ * denom_);
        denom_ = denom_ * rhs.denom_;
        fix_signs();
        reduce();
      }
    }
  }

  return *this;
}

const rational& rational::operator-=(const rational &rhs)
{
  if (!isNaN()) {
    if (rhs.isNaN()) {
      // Set to NaN
      denom_ = 0;
      fix_signs();
    } else if (!rhs.isNull()) {
      if (isNull()) {
        numer_ = -rhs.numer_;
        denom_ = rhs.denom_;
      } else {
        numer_ = (numer_ * rhs.denom_) - (rhs.numer_ * denom_);
        denom_ = denom_ * rhs.denom_;
        fix_signs();
        reduce();
      }
    }
  }

  return *this;
}

const rational& rational::operator/=(const rational &rhs)
{
  if (!isNaN()) {
    if (rhs.isNaN()) {
      // Set to NaN
      denom_ = 0;
      fix_signs();
    } else {
      numer_ = numer_ * rhs.denom_;
      denom_ = denom_ * rhs.numer_;
      fix_signs();
      reduce();
    }
  }

  return *this;
}

const rational& rational::operator*=(const rational &rhs)
{
  if (!isNaN()) {
    if (rhs.isNaN()) {
      denom_ = 0;
      fix_signs();
    } else {
      numer_ = numer_ * rhs.numer_;
      denom_ = denom_ * rhs.denom_;
      fix_signs();
      reduce();
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
  if (isNaN() || rhs.isNaN()) {
    return false;
  }

  if (isNull() && rhs.isNull()) {
    return false;
  }

  if (rhs == RATIONAL_MAX
      || *this == RATIONAL_MIN) {
    // We will always wither be LESS THAN (true) or EQUAL (false)
    return (*this != rhs);
  }

  if (*this == RATIONAL_MAX
      || rhs == RATIONAL_MIN) {
    // We will always be GREATER THAN (false) or EQUAL (false)
    return false;
  }

  if (!isNull() && rhs.isNull()) {
    return (numer_ * denom_ < intType(0));
  }

  if (isNull() && !rhs.isNull()) {
    return !(rhs.numer_ * rhs.denom_ < intType(0));
  }

  return ((numer_ * rhs.denom_) < (denom_ * rhs.numer_));
}

bool rational::operator<=(const rational &rhs) const
{
  if (isNaN() || rhs.isNaN()) {
    return false;
  }

  if (isNull() && rhs.isNull()) {
    return true;
  }

  if (rhs == RATIONAL_MAX
      || *this == RATIONAL_MIN) {
    // We will always wither be LESS THAN (true) or EQUAL (true)
    return true;
  }

  if (*this == RATIONAL_MAX
      || rhs == RATIONAL_MIN) {
    // We will always be GREATER THAN (false) or EQUAL (true)
    return rhs == *this;
  }

  if (!isNull() && rhs.isNull()) {
    return (numer_ * denom_ < intType(0));
  }

  if (isNull() && !rhs.isNull()) {
    return !(rhs.numer_ * rhs.denom_ < intType(0));
  }

  return ((numer_ * rhs.denom_) <= (denom_ * rhs.numer_));
}

bool rational::operator>(const rational &rhs) const
{
  return rhs < *this;
}

bool rational::operator>=(const rational &rhs) const
{
  return rhs <= *this;
}

bool rational::operator==(const rational &rhs) const
{
  if (isNaN() || rhs.isNaN()) {
    return false;
  }

  return (numer_ == rhs.numer_ && denom_ == rhs.denom_);
}

bool rational::operator!=(const rational &rhs) const
{
  return !(*this == rhs);
}

const rational& rational::operator+() const
{
  return *this;
}

rational rational::operator-() const
{
  return rational(numer_, -denom_);
}

bool rational::operator!() const
{
  return !numer_;
}

//IO

std::ostream& operator<<(std::ostream &out, const rational &value)
{
  out << value.numer_;

  if (value.denom_ != 1) {
    out << '/' << value.denom_;
    return out;
  }

  return out;
}

std::istream& operator>>(std::istream &in, rational &value)
{
  in >> value.numer_;
  value.denom_ = 1;

  char ch;
  in.get(ch);

  if(!in.eof()) {
    if(ch == '/') {
      in >> value.denom_;
      value.fix_signs();
      value.reduce();
    } else {
      in.putback(ch);
    }
  }
  return in;
}

uint qHash(const rational &r, uint seed)
{
  return ::qHash(r.toDouble(), seed);
}

}

QDebug operator<<(QDebug debug, const olive::rational &r)
{
  if (r.isNaN()) {
    return debug.space() << "NaN";
  } else {
    return debug.space() << r.toDouble();
  }
}
