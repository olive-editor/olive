//Copyright 2015 Adam Quintero
//This program is distributed under the terms of the GNU General Public License.

#include "rational.h"

#include <cmath>

using namespace std;

rational::rational(const intType& num)
    : numer(num)
      , denom(1)
{}

rational::rational(const intType& num, const intType& den)
    : numer(num)
      , denom(den)
{
    fixSigns();
    reduce();
}
rational::rational(const AVRational &r) :
    numer(r.num),
    denom(r.den)
{
}

//Function: print number to cout

void rational::print(ostream &out) const
{
    out << this->numer << "/" << this->denom;
}

//Function: ensures denom >= 0

void rational::fixSigns()
{
    numer *= 1 - 2 * std::signbit(denom);
    denom = std::abs(denom);
}

//Function: ensures lowest form

void rational::reduce()
{
    intType d = gcd(std::abs(numer), denom);
    numer /= d;
    denom /= d;
}

//Function: finds greatest common denominator

intType rational::gcd(const intType &x, const intType &y)
{
    return y == 0 ? x : gcd(y, x % y);
}

//Function: convert to double

double rational::toDouble() const
{
    if(denom != 0)
        return static_cast<double>(numer) / static_cast<double>(denom);
    else
        return std::nan("");
}

AVRational rational::toAVRational() const
{
    AVRational r;

    r.num = static_cast<int>(numer);
    r.den = static_cast<int>(denom);

    return r;
}

rational rational::flipped() const
{
    return rational(denom, numer);
}

bool rational::isNull() const
{
    return denominator() == 0;
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

rational& rational::operator=(const rational &rhs)
{
    if(this != &rhs)
    {
        numer = rhs.numer;
        denom = rhs.denom;
    }
    return *this;
}

rational& rational::operator+=(const rational &rhs)
{
    intType dn = denom * rhs.denom;
    intType nn = numer * rhs.denom + rhs.numer * denom;
    numer = nn;
    denom = dn;
    reduce();
    return *this;
}

rational& rational::operator-=(const rational &rhs)
{
    intType dn = denom * rhs.denom;
    intType nn = numer * rhs.denom - rhs.numer * denom;
    numer = nn;
    denom = dn;
    reduce();
    return *this;
}

rational& rational::operator/=(const rational &rhs)
{
    numer = numer * rhs.denom;
    denom = denom * rhs.numer;
    fixSigns();
    reduce();
    return *this;
}

rational& rational::operator*=(const rational &rhs)
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
    if (this->isNull() or rhs.isNull())
    {
        return false;
    }
    return numer * rhs.denom < denom * rhs.numer;
}

bool rational::operator<=(const rational &rhs) const
{
    if (this->isNull() or rhs.isNull())
    {
        return false;
    }
    return *this < rhs || *this == rhs;
}

bool rational::operator>(const rational &rhs) const
{
    if (this->isNull() or rhs.isNull())
    {
        return false;
    }
    return rhs < *this;
}

bool rational::operator>=(const rational &rhs) const
{
    if (this->isNull() or rhs.isNull())
    {
        return false;
    }
    return *this == rhs || rhs < *this;
}

bool rational::operator==(const rational &rhs) const
{
    if (this->isNull() or rhs.isNull())
    {
        return false;
    }
    return (numer == rhs.numer && denom == rhs.denom);
}

bool rational::operator!=(const rational &rhs) const
{
    return !(*this == rhs);
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

QDebug operator<<(QDebug debug, const rational &r)
{
    debug.nospace() << r.numerator() << "/" << r.denominator();
    return debug.space();
}

uint qHash(const rational &r, uint seed)
{
  return qHash(r.toDouble(), seed);
}
