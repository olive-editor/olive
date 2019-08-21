//Copyright 2015 Adam Quintero
//This program is distributed under the terms of the GNU General Public License.

#include "rational.h"

int rational::activeInstances = 0;

rational::rational(const AVRational &r) :
  numer(r.num),
  denom(r.den)
{
}

rational::~rational()
{
  activeInstances--;
}

//Function: print number to cout

void rational::print(ostream &out) const
{
  out << this->numer << "/" << this->denom;
}

//Function: ensures denom >= 0

void rational::fixSigns()
{
  if(denom < 0)
    {
      denom = -denom;
      numer = -numer;
    }
  if(numer == intType(0) || denom == intType(0))
    {
      numer = intType(0);
      denom = intType(0);
    }
}

//Function: ensures lowest form

void rational::reduce()
{
  intType d = 1;

  if(denom != 0 && numer !=0)
    d = gcd(numer, denom);

  if(d > 1)
    {
      numer /= d;
      denom /= d;
    }
}

//Function: finds greatest common denominator

intType rational::gcd(intType &x, intType &y)
{
  if(y == 0)
    return x;
  else
    {
      intType tmp = x % y;

      return gcd(y, tmp);
    }
}

//Function: convert to double

double rational::toDouble() const
{
  if(denom != 0)
    return static_cast<double>(numer) / static_cast<double>(denom);
  else
    return static_cast<double>(0);
}

rational rational::flipped() const
{
  return rational(denom, numer);
}

bool rational::isNull() const
{
  return denominator() == 0;
}

//Function: get active instances

int rational::getActiveInstances()
{
  return activeInstances;
}

const intType &rational::numerator() const
{
  return numer;
}

const intType &rational::denominator() const
{
  return denom;
}

//Assignment Operators

const rational& rational::operator=(const rational &rhs)
{
  if(this != &rhs)
    {
      numer = rhs.numer;
      denom = rhs.denom;
    }
  return *this;
}

const rational& rational::operator+=(const rational &rhs)
{
  if(numer * denom == intType(0) && rhs.numer * rhs.denom == intType(0))
    {
      numer = intType(0);
      denom = intType(0);
    }
  else
    if(numer * denom != intType(0) && rhs.numer * rhs.denom == intType(0))
      {

      }
    else
      if(numer * denom == intType(0) && rhs.numer * rhs.denom != intType(0))
        {
          numer = rhs.numer;
          denom = rhs.denom;
        }
      else
        {
          numer = (numer * rhs.denom) + (rhs.numer * denom);
          denom = denom * rhs.denom;
          fixSigns();
          reduce();
        }
  return *this;
}

const rational& rational::operator-=(const rational &rhs)
{
  if(numer * denom == intType(0) && rhs.numer * rhs.denom == intType(0))
    {
      numer = intType(0);
      denom = intType(0);
    }
  else
    if(numer * denom != intType(0) && rhs.numer * rhs.denom == intType(0))
      {

      }
    else
      if(numer * denom == intType(0) && rhs.numer * rhs.denom != intType(0))
        {
          numer = -(rhs.numer);
          denom = rhs.denom;
        }
      else
        {
          numer = (numer * rhs.denom) - (rhs.numer * denom);
          denom = denom * rhs.denom;
          fixSigns();
          reduce();
        }
  return *this;
}

const rational& rational::operator/=(const rational &rhs)
{
  numer = numer * rhs.denom;
  denom = denom * rhs.numer;
  fixSigns();
  reduce();
  return *this;
}

const rational& rational::operator*=(const rational &rhs)
{
  numer = numer * rhs.numer;
  denom = denom * rhs.denom;
  fixSigns();
  reduce();
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
  if(numer * denom == intType(0) && rhs.numer * rhs.denom == intType(0))
    return false;
  else
    if(numer * denom != intType(0) && rhs.numer * rhs.denom == intType(0))
      {
        if(numer * denom < intType(0))
          return true;
        else
          return false;
      }
    else
      if(numer * denom == intType(0) && rhs.numer * rhs.denom != intType(0))
        {
          if(rhs.numer * rhs.denom < intType(0))
            return false;
          else
            return true;
        }
      else
        return ((numer * rhs.denom) < (denom * rhs.numer));
}

bool rational::operator<=(const rational &rhs) const
{
  if(numer * denom == intType(0) && rhs.numer * rhs.denom == intType(0))
    return true;
  else
    if(numer * denom != intType(0) && rhs.numer * rhs.denom == intType(0))
      {
        if(numer * denom < intType(0))
          return true;
        else
          return false;
      }
    else
      if(numer * denom == intType(0) && rhs.numer * rhs.denom != intType(0))
        {
          if(rhs.numer * rhs.denom < intType(0))
            return false;
          else
            return true;
        }
      else
        return ((numer * rhs.denom) <= (denom * rhs.numer));
}

bool rational::operator>(const rational &rhs) const
{
  if(numer * denom == intType(0) && rhs.numer * rhs.denom == intType(0))
    return false;
  else
    if(numer * denom != intType(0) && rhs.numer * rhs.denom == intType(0))
      {
        if(numer * denom > intType(0))
          return true;
        else
          return false;
      }
    else
      if(numer * denom == intType(0) && rhs.numer * rhs.denom != intType(0))
        {
          if(rhs.numer * rhs.denom > intType(0))
            return false;
          else
            return true;
        }
      else
        return ((numer * rhs.denom) > (denom * rhs.numer));
}

bool rational::operator>=(const rational &rhs) const
{
  if(numer * denom == intType(0) && rhs.numer * rhs.denom == intType(0))
    return true;
  else
    if(numer * denom != intType(0) && rhs.numer * rhs.denom == intType(0))
      {
        if(numer * denom > intType(0))
          return true;
        else
          return false;
      }
    else
      if(numer * denom == intType(0) && rhs.numer * rhs.denom != intType(0))
        {
          if(rhs.numer * rhs.denom > intType(0))
            return false;
          else
            return true;
        }
      else

  return ((numer * rhs.denom) >= (denom * rhs.numer));
}

bool rational::operator==(const rational &rhs) const
{
  return (numer == rhs.numer && denom == rhs.denom);
}

bool rational::operator!=(const rational &rhs) const
{
  return (numer != rhs.numer) || (denom != rhs.denom);

}

//Unary operators

const rational& rational::operator++()
{
  numer += denom;
  return *this;
}

rational rational::operator++(int)
{
  rational tmp = *this;
  numer += denom;
  return tmp;
}

const rational& rational::operator--()
{
  numer -= denom;
  return *this;

}

rational rational::operator--(int)
{
  rational tmp;
  numer -= denom;
  return tmp;
}

const rational& rational::operator+() const
{
  return *this;
}

rational rational::operator-() const
{
  return rational(numer, -denom);
}

bool rational::operator!() const
{
  return !numer;
}

//IO

ostream& operator<<(ostream &out, const rational &value)
{
  out << value.numer;
  if(value.denom != 1)
    {
      out << '/' << value.denom;
      return out;
    }
  return out;
}

istream& operator>>(istream &in, rational &value)
{
  in >> value.numer;
  value.denom = 1;

  char ch;
  in.get(ch);

  if(!in.eof())
    {
      if(ch == '/')
        {
          in >> value.denom;
          value.fixSigns();
          value.reduce();
        }
      else
        in.putback(ch);
    }
  return in;
}

