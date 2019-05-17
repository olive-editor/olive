#include "rational.h"

rational::rational(const int &numerator) :
  numerator_(numerator),
  denominator_(1)
{
  if (numerator_ == 0) {
    denominator_ = 0;
  }
}

rational::rational(const int &numerator, const int &denominator) :
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
  int d = 1;

  if(denominator_ != 0 && numerator_ != 0) {
    d = GreatestCommonDenominator(numerator_, denominator_);
  }

  if(d > 1) {
    numerator_ /= d;
    denominator_ /= d;
  }
}

int rational::GreatestCommonDenominator(const int& x, const int& y)
{
  if (y == 0) {
    return x;
  } else {
    int tmp = x % y;
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
