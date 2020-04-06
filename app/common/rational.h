//Copyright 2015 Adam Quintero
//This program is distributed under the terms of the GNU General Public License.

// Adapted by MattKC for the Olive Video Editor (2019)

#ifndef RATIONAL_H
#define RATIONAL_H
#include <iostream>

#include <QDebug>
#include <QMetaType>

extern "C" {
#include <libavformat/avformat.h>
}

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

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
  rational(const intType &numerator = 0)
    :numer(numerator), denom(1)
  {
    if(numer == 0)
      denom = 0;
  }

  rational(const intType &numerator, const intType &denominator)
    :numer(numerator), denom(denominator)
  {
    validateConstructor();
  }

  rational(const rational &rhs) = default;

  rational(const AVRational& r);

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
  intType numer;
  intType denom;

  void validateConstructor();

  //Function: ensures denom >= 0
  void fixSigns();
  //Function: ensures lowest form
  void reduce();
  //Function: finds greatest common denominator
  intType gcd(intType &x, intType &y);
};

// We define these limits at 32-bit to try avoiding integer overflow
#define RATIONAL_MIN rational(INT32_MIN, 1)
#define RATIONAL_MAX rational(INT32_MAX, 1)

uint qHash(const rational& r, uint seed);

OLIVE_NAMESPACE_EXIT

QDebug operator<<(QDebug debug, const OLIVE_NAMESPACE::rational& r);

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::rational)

#endif // RATIONAL_H
