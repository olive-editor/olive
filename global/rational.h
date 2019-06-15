#ifndef RATIONAL_H
#define RATIONAL_H

// Adapted from https://github.com/angularadam/Qt-Class-rational used in compliance with the GNU General Public License

#include <iostream>

extern "C" {
  #include <libavformat/avformat.h>
}

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
private:
  int64_t numerator_;
  int64_t denominator_;

  void FixSigns();
  void Reduce();
  int64_t GreatestCommonDenominator(const int64_t &x, const int64_t &y);
};

#endif // RATIONAL_H
