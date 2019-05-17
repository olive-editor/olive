#ifndef RATIONAL_H
#define RATIONAL_H

// Adapted from https://github.com/angularadam/Qt-Class-rational used in compliance with the GNU General Public License

#include <iostream>
#include <libavformat/avformat.h>

class rational {
public:
  // Constructors
  rational(const int& numerator = 0);
  rational(const int& numerator, const int& denominator);
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
private:
  int numerator_;
  int denominator_;

  void FixSigns();
  void Reduce();
  int GreatestCommonDenominator(const int &x, const int &y);
};

#endif // RATIONAL_H
