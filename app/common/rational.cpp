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

#include "rational.h"

rational::rational(const int64_t &numerator) :
  numerator_(numerator),
  denominator_(1)
{
  if (numerator_ == 0) {
    denominator_ = 0;
  }
}

rational::rational(const int64_t &numerator, const int64_t &denominator) :
  numerator_(numerator),
  denominator_(denominator)
{
  if (denominator_ != 0) {

    if (numerator_ != 0) {
      FixSigns();
      Reduce();
    } else {
      denominator_ = 0;
    }

  } else {
    numerator_ = 0;
  }
}

rational::rational(const AVRational &r) :
  numerator_(r.num),
  denominator_(r.den)
{
}

rational::rational(const rational &r) :
  numerator_(r.numerator_),
  denominator_(r.denominator_)
{
}

const rational &rational::operator=(const rational &r)
{
  if (this != &r) {
    numerator_ = r.numerator_;
    denominator_ = r.denominator_;
  }

  return *this;
}

const rational &rational::operator+=(const rational &r)
{
  if (numerator_ * denominator_ == 0 && r.numerator_ * r.denominator_ == 0) {
    numerator_ = 0;
    denominator_ = 0;
  } else {
    if(numerator_ * denominator_ != 0 && r.numerator_ * r.denominator_ == 0) {
      // The other rational == 0, no addition needs to be done
    } else {
      if(numerator_ * denominator_ == 0 && r.numerator_ * r.denominator_ != 0)
      {
        numerator_ = r.numerator_;
        denominator_ = r.denominator_;
      }
      else
      {
        numerator_ = (numerator_ * r.denominator_) + (r.numerator_ * denominator_);
        denominator_ = denominator_ * r.denominator_;
        FixSigns();
        Reduce();
      }
    }
  }
  return *this;
}

const rational &rational::operator-=(const rational &r)
{
  if(numerator_ * denominator_ == 0 && r.numerator_ * r.numerator_ == 0) {
    numerator_ = 0;
    denominator_ = 0;
  } else {
    if(numerator_ * denominator_ != 0 && r.numerator_ * r.denominator_ == 0) {

    } else {
      if(numerator_ * denominator_ == 0 && r.numerator_ * r.denominator_ != 0) {
        numerator_ = -(r.numerator_);
        denominator_ = r.denominator_;
      } else {
        numerator_ = (numerator_ * r.denominator_) - (r.numerator_ * denominator_);
        numerator_ = denominator_ * r.denominator_;
        FixSigns();
        Reduce();
      }
    }
  }
  return *this;
}

const rational &rational::operator/=(const rational &r)
{
  numerator_ = numerator_ * r.denominator_;
  denominator_ = denominator_ * r.numerator_;
  FixSigns();
  Reduce();
  return *this;
}

const rational &rational::operator*=(const rational &r)
{
  numerator_ = numerator_ * r.numerator_;
  denominator_ = denominator_ * r.denominator_;
  FixSigns();
  Reduce();
  return *this;
}

rational rational::operator+(const rational &r) const
{
  rational result(*this);
  result += r;
  return result;
}

rational rational::operator-(const rational &r) const
{
  rational result(*this);
  result -= r;
  return result;
}

rational rational::operator/(const rational &r) const
{
  rational result(*this);
  result /= r;
  return result;
}

rational rational::operator*(const rational &r) const
{
  rational result(*this);
  result *= r;
  return result;
}

bool rational::operator<(const rational &r) const
{
  if (numerator_ * denominator_ == 0 && r.numerator_ * r.denominator_ == 0) {
    return false;
  } else {
    if (numerator_ * denominator_ != 0 && r.numerator_ * r.denominator_ == 0) {
      if(numerator_ * denominator_ < 0)
        return true;
      else
        return false;
    }
    else {
      if (numerator_ * denominator_ == 0 && r.numerator_ * r.denominator_ != 0) {
        if (r.numerator_ * r.denominator_ < 0) {
          return false;
        } else {
          return true;
        }
      } else {
        return ((numerator_ * r.denominator_) < (denominator_ * r.numerator_));
      }
    }
  }
}

bool rational::operator<=(const rational &r) const
{
  if(numerator_ * denominator_ == 0 && r.numerator_ * r.denominator_ == 0) {
    return true;
  } else {
    if(numerator_ * denominator_ != 0 && r.numerator_ * r.denominator_ == 0) {
      if(numerator_ * denominator_ < 0) {
        return true;
      } else {
        return false;
      }
    } else {
      if(numerator_ * denominator_ == 0 && r.numerator_ * r.denominator_ != 0) {
        if(r.numerator_ * r.denominator_ < 0) {
          return false;
        } else {
          return true;
        }
      } else {
        return ((numerator_ * r.denominator_) <= (denominator_ * r.numerator_));
      }
    }
  }
}

bool rational::operator>(const rational &r) const
{
  if(numerator_ * denominator_ == 0 && r.numerator_ * r.denominator_ == 0) {
    return false;
  } else {
    if(numerator_ * denominator_ != 0 && r.numerator_ * r.denominator_ == 0) {
      if(numerator_ * denominator_ > 0) {
        return true;
      } else {
        return false;
      }
    } else {
      if(numerator_ * denominator_ == 0 && r.numerator_ * r.denominator_ != 0) {
        if(r.numerator_ * r.denominator_ > 0) {
          return false;
        } else {
          return true;
        }
      } else {
        return ((numerator_ * r.denominator_) > (denominator_ * r.numerator_));
      }
    }
  }
}

bool rational::operator>=(const rational &r) const
{
  if(numerator_ * denominator_ == 0 && r.numerator_ * r.denominator_ == 0) {
    return true;
  } else {
    if(numerator_ * denominator_ != 0 && r.numerator_ * r.denominator_ == 0) {
      if(numerator_ * denominator_ > 0) {
        return true;
      } else {
        return false;
      }
    } else {
      if(numerator_ * denominator_ == 0 && r.numerator_ * r.denominator_ != 0) {
        if(r.numerator_ * r.denominator_ > 0) {
          return false;
        } else {
          return true;
        }
      } else {
        return ((numerator_ * r.denominator_) >= (denominator_ * r.numerator_));
      }
    }
  }
}

bool rational::operator==(const rational &r) const
{
  return (numerator_ == r.numerator_ && denominator_ == r.denominator_);
}

bool rational::operator!=(const rational &r) const
{
  return (numerator_ != r.numerator_) || (denominator_ != r.denominator_);
}

const rational &rational::operator++()
{
  numerator_ += denominator_;
  return *this;
}

rational rational::operator++(int)
{
  rational tmp = *this;
  numerator_ += denominator_;
  return tmp;
}

const rational &rational::operator--()
{
  numerator_ -= denominator_;
  return *this;
}

rational rational::operator--(int)
{
  rational tmp;
  numerator_ -= denominator_;
  return tmp;
}

const rational &rational::operator+() const
{
  return *this;
}

rational rational::operator-() const
{
  return rational(numerator_, -denominator_);
}

bool rational::operator!() const
{
  return !numerator_;
}

double rational::ToDouble() const
{
  if (denominator_ == 0) {
    return 0;
  } else {
    return static_cast<double>(numerator_)/static_cast<double>(denominator_);
  }
}

const int64_t &rational::numerator() const
{
  return numerator_;
}

const int64_t &rational::denominator() const
{
  return denominator_;
}

rational rational::flipped() const
{
  return rational(denominator_, numerator_);
}

void rational::FixSigns()
{
  // Ensures denominator is always positive (while numerator can be positive or negative)
  if(denominator_ < 0) {
    denominator_ = -denominator_;
    numerator_ = -numerator_;
  }

  // Ensures if either are zero, they're both zero
  if(numerator_ == 0 || denominator_ == 0) {
    numerator_ = 0;
    denominator_ = 0;
  }
}

void rational::Reduce()
{
  int64_t d = 1;

  if(denominator_ != 0 && numerator_ != 0) {
    d = GreatestCommonDenominator(numerator_, denominator_);
  }

  if(d > 1) {
    numerator_ /= d;
    denominator_ /= d;
  }
}

int64_t rational::GreatestCommonDenominator(const int64_t& x, const int64_t& y)
{
  if (y == 0) {
    return x;
  } else {
    int64_t tmp = x % y;
    return GreatestCommonDenominator(y, tmp);
  }
}

std::ostream &operator<<(std::ostream &out, const rational &value)
{
  out << value.numerator_;
  if(value.denominator_ != 1)
  {
    out << '/' << value.denominator_;
    return out;
  }
  return out;
}

std::istream &operator>>(std::istream &in, rational &value)
{
  in >> value.numerator_;
  value.denominator_ = 1;

  char ch;
  in.get(ch);

  if(!in.eof())
  {
    if(ch == '/')
    {
      in >> value.denominator_;
      value.FixSigns();
      value.Reduce();
    }
    else
      in.putback(ch);
  }
  return in;
}
