//Copyright 2015 Adam Quintero
//This program is distributed under the terms of the GNU General Public License.

// Adapted by MattKC for the Olive Video Editor (2019)

#ifndef RATIONAL_H
#define RATIONAL_H
#include <iostream>

#ifdef USE_OTIO
#include <opentime/rationalTime.h>
#endif

#include <QDebug>
#include <QMetaType>

extern "C" {
#include <libavformat/avformat.h>
}

#include "common/define.h"

namespace olive {

typedef int64_t intType;
/*
 * Zero Handling
 * 0/0        = 0
 * 0/non-zero = 0
 * non-zero/0 = 0
*/
class rational
{
public:
  //constructors
  rational(const intType &numerator = 0) :
    numer_(numerator)
  {
    if (numer_ == 0) {
      denom_ = 0;
    } else {
      denom_ = 1;
    }
  }

  rational(const intType &numerator, const intType &denominator) :
    numer_(numerator),
    denom_(denominator)
  {
    fix_signs();
    reduce();
  }

  rational(const rational &rhs) = default;

  rational(const AVRational& r) :
    numer_(r.num),
    denom_(r.den)
  {
    fix_signs();
    reduce();
  }

  static rational fromDouble(const double& flt);

  static rational fromString(const QString& str);

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
  const rational& operator++(); //prefix
  rational operator++(int);     //postfix
  const rational& operator--(); //prefix
  rational operator--(int);     //postfix
  const rational& operator+() const;
  rational operator-() const;
  bool operator!() const;

  //Function: convert to double
  double toDouble() const;

  AVRational toAVRational() const;

#ifdef USE_OTIO
  opentime::RationalTime toRationalTime() const;
#endif

  // Produce "flipped" version
  rational flipped() const;

  // Returns whether the rational is null or not
  bool isNull() const;

  //Function: print number to cout
  void print(std::ostream &out = std::cout) const;

  //IO
  friend std::ostream& operator<<(std::ostream &out, const rational &value);
  friend std::istream& operator>>(std::istream &in, rational &value);

  const intType& numerator() const;
  const intType& denominator() const;

  QString toString() const;

private:
  //numerator and denominator
  intType numer_;
  intType denom_;

  //Function: ensures denom >= 0
  void fix_signs();
  //Function: ensures lowest form
  void reduce();
  //Function: finds greatest common denominator
  static intType gcd(const intType &x, const intType &y);
};

#define RATIONAL_MIN rational(INT64_MIN, 1)
#define RATIONAL_MAX rational(INT64_MAX, 1)

uint qHash(const rational& r, uint seed);

}

QDebug operator<<(QDebug debug, const olive::rational& r);

Q_DECLARE_METATYPE(olive::rational)

#endif // RATIONAL_H
