/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

// Adapted from https://github.com/angularadam/Qt-Class-rational used in compliance with the GNU General Public License

#include <iostream>

/**
 * Including AVFormat to support converting from AVRational to Olive's rational class
 */
extern "C" {
  #include <libavformat/avformat.h>
}

/**
 * @brief The Rational class
 *
 * A rational (numerator/denominator) class with C++ operations built in for ease of use.
 *
 * Rationals in Olive most frequently represent timing information to easily handle timing in various different
 * frame/sample rates without the inaccuracy/rounding errors of a floating point type.
 */
class rational {
public:
  // Constructors
  rational(const int64_t& numerator = 0);
  rational(const int64_t& numerator, const int64_t& denominator);
  rational(const AVRational& r); // Auto-convert from an FFmpeg AVRational
  rational(const rational& r);

  // Assignment Operators
  const rational& operator=(const rational& r);
  const rational& operator+=(const rational& r);
  const rational& operator-=(const rational& r);
  const rational& operator/=(const rational& r);
  const rational& operator*=(const rational& r);

  // Math Operators
  rational operator+(const rational& r) const;
  rational operator-(const rational& r) const;
  rational operator/(const rational& r) const;
  rational operator*(const rational& r) const;

  // Relational and Equality Operators
  bool operator<(const rational &r) const;
  bool operator<=(const rational &r) const;
  bool operator>(const rational &r) const;
  bool operator>=(const rational &r) const;
  bool operator==(const rational &r) const;
  bool operator!=(const rational &r) const;

  //Unary operators
  const rational& operator++(); //prefix
  rational operator++(int);     //postfix
  const rational& operator--(); //prefix
  rational operator--(int);     //postfix
  const rational& operator+() const;
  rational operator-() const;
  bool operator!() const;

  // IO
  friend std::ostream& operator<<(std::ostream &out, const rational& value);
  friend std::istream& operator>>(std::istream &in, rational& value);

  // Convert to double
  double ToDouble() const;

  // Specific values
  const int64_t& numerator();
  const int64_t& denominator();
private:
  int64_t numerator_;
  int64_t denominator_;

  void FixSigns();
  void Reduce();
  int64_t GreatestCommonDenominator(const int64_t &x, const int64_t &y);
};

#endif // RATIONAL_H
