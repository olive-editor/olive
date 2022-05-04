/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef RATIONAL_H
#define RATIONAL_H

extern "C" {
#include <libavutil/rational.h>
}

#include <iostream>
#include <QDebug>
#include <QMetaType>

#ifdef USE_OTIO
#include <opentime/rationalTime.h>
#endif

#include "common/define.h"

namespace olive {

class rational
{
public:
  rational(const int &numerator = 0)
  {
    r_.num = numerator;
    r_.den = 1;
  }

  rational(const int &numerator, const int &denominator)
  {
    r_.num = numerator;
    r_.den = denominator;

    FixSigns();
    Reduce();
  }

  rational(const rational &rhs) = default;

  rational(const AVRational& r)
  {
    r_ = r;

    FixSigns();
  }

  static rational fromDouble(const double& flt, bool *ok = nullptr);
  static rational fromString(const QString& str, bool* ok = nullptr);

  static const rational NaN;

  //Assignment Operators
  const rational& operator=(const rational &rhs);
  const rational& operator+=(const rational &rhs);
  const rational& operator-=(const rational &rhs);
  const rational& operator/=(const rational &rhs);
  const rational& operator*=(const rational &rhs);

  //Binary math operators
  rational operator+(const rational &rhs) const;
  rational operator-(const rational &rhs) const;
  rational operator/(const rational &rhs) const;
  rational operator*(const rational &rhs) const;

  //Relational and equality operators
  bool operator<(const rational &rhs) const;
  bool operator<=(const rational &rhs) const;
  bool operator>(const rational &rhs) const;
  bool operator>=(const rational &rhs) const;
  bool operator==(const rational &rhs) const;
  bool operator!=(const rational &rhs) const;

  //Unary operators
  const rational& operator+() const { return *this; }
  rational operator-() const { return rational(r_.num, -r_.den); }
  bool operator!() const { return !r_.num; }

  //Function: convert to double
  double toDouble() const;

  AVRational toAVRational() const;

#ifdef USE_OTIO
  static rational fromRationalTime(const opentime::RationalTime &t)
  {
    // Is this the best way to do this?
    return fromDouble(t.to_seconds());
  }

  // Convert Olive rationals to opentime rationals with the given framerate (defaults to 24)
  opentime::RationalTime toRationalTime(double framerate = 24) const;
#endif

  // Produce "flipped" version
  rational flipped() const;
  void flip();

  // Returns whether the rational is valid but equal to zero or not
  //
  // A NaN is always a null, but a null is not always a NaN
  bool isNull() const { return r_.num == 0; }

  // Returns whether this rational is not a valid number (denominator == 0)
  bool isNaN() const { return r_.den == 0; }

  const int& numerator() const { return r_.num; }
  const int& denominator() const { return r_.den; }

  QString toString() const;

  friend std::ostream& operator<<(std::ostream &out, const rational &value)
  {
    out << value.r_.num << '/' << value.r_.den;

    return out;
  }

private:
  void FixSigns();
  void Reduce();

  AVRational r_;

};

#define RATIONAL_MIN rational(INT_MIN)
#define RATIONAL_MAX rational(INT_MAX)

uint qHash(const rational& r, uint seed = 0);

}

QDebug operator<<(QDebug debug, const olive::rational& r);

Q_DECLARE_METATYPE(olive::rational)

#endif // RATIONAL_H
