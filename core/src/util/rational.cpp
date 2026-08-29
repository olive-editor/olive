/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC
  Modifications Copyright (C) 2025 mikesolar

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

#include "util/rational.h"

#include <math.h>
#include <stdint.h>

#include <algorithm>
#include <climits>
#include <limits>

#include "util/fractionutils.h"
#include "util/stringutils.h"

namespace olive::core
{

const rational rational::NaN = rational(0, 0);

rational rational::fromDouble(const double &flt, bool *ok)
{
	if (isnan(flt)) {
		// Return NaN rational
		if (ok)
			*ok = false;
		return NaN;
	}

	if (fabs(flt) > double(INT_MAX) + 3.0) {
		// Value is out of range for a rational, return NaN
		if (ok) {
			*ok = false;
		}
		return NaN;
	}

	// Continued fraction conversion (ported from FFmpeg's av_d2q)
	int exponent;
	frexp(flt, &exponent);
	exponent = std::max(exponent - 1, 0);
	int64_t den = 1LL << (62 - exponent);
	int64_t num = int64_t(floor(flt * den + 0.5));

	int64_t rnum = num, rden = den;
	ReduceFraction(rnum, rden, INT_MAX);

	if ((!rnum || !rden) && flt) {
		// Value was too small to represent above, retry with maximum precision
		rnum = int64_t(flt * double(INT64_MAX));
		rden = INT64_MAX;
		ReduceFraction(rnum, rden, INT_MAX);
	}

	if (rden == 0) {
		// If den == 0, we were unable to convert to a rational
		if (ok) {
			*ok = false;
		}
		return NaN;
	}

	// Otherwise, assume we received a real rational
	if (ok) {
		*ok = true;
	}

	return rational(int(rnum), int(rden));
}

rational rational::fromString(const std::string &str, bool *ok)
{
	std::vector<std::string> elements = StringUtils::split(str, '/');

	switch (elements.size()) {
	case 1:
		return rational(StringUtils::to_int(elements.front(), ok));
	case 2:
		return rational(StringUtils::to_int(elements.at(0), ok),
						StringUtils::to_int(elements.at(1), ok));
	default:
		// Returns NaN with ok set to false
		if (ok) {
			*ok = false;
		}
		return NaN;
	}
}

//Function: convert to double

double rational::toDouble() const
{
	if (den_ != 0) {
		return double(num_) / double(den_);
	} else {
		return std::numeric_limits<double>::quiet_NaN();
	}
}

#ifdef USE_OTIO
opentime::RationalTime rational::toRationalTime(double framerate) const
{
	// Is this the best way of doing this?
	// Olive can store rationals as 0/0 which causes errors in OTIO
	opentime::RationalTime time =
		opentime::RationalTime(num_, den_ == 0 ? 1 : den_);
	return time.rescaled_to(framerate);
}
#endif

rational rational::flipped() const
{
	rational r = *this;
	r.flip();
	return r;
}

void rational::flip()
{
	if (!isNull()) {
		std::swap(den_, num_);
		fix_signs();
	}
}

std::string rational::toString() const
{
	return StringUtils::format("%d/%d", num_, den_);
}

void rational::fix_signs()
{
	if (den_ < 0) {
		// Normalize so that denominator is always positive
		den_ = -den_;
		num_ = -num_;
	} else if (den_ == 0) {
		// Normalize to 0/0 (aka NaN) if denominator is zero
		num_ = 0;
	} else if (num_ == 0) {
		// Normalize to 0/1 if numerator is zero
		den_ = 1;
	}
}

void rational::reduce()
{
	int64_t n = num_, d = den_;
	ReduceFraction(n, d, INT_MAX);
	num_ = int(n);
	den_ = int(d);
}

//Assignment Operators

const rational &rational::operator=(const rational &rhs)
{
	num_ = rhs.num_;
	den_ = rhs.den_;
	return *this;
}

const rational &rational::operator+=(const rational &rhs)
{
	if (*this == RATIONAL_MIN || *this == RATIONAL_MAX || rhs == RATIONAL_MIN ||
		rhs == RATIONAL_MAX) {
		*this = NaN;
	} else if (!isNaN()) {
		if (rhs.isNaN()) {
			*this = NaN;
		} else {
			int64_t n = num_ * int64_t(rhs.den_) + rhs.num_ * int64_t(den_);
			int64_t d = den_ * int64_t(rhs.den_);
			ReduceFraction(n, d, INT_MAX);
			num_ = int(n);
			den_ = int(d);
			fix_signs();
		}
	}

	return *this;
}

const rational &rational::operator-=(const rational &rhs)
{
	if (*this == RATIONAL_MIN || *this == RATIONAL_MAX || rhs == RATIONAL_MIN ||
		rhs == RATIONAL_MAX) {
		*this = NaN;
	} else if (!isNaN()) {
		if (rhs.isNaN()) {
			*this = NaN;
		} else {
			int64_t n = num_ * int64_t(rhs.den_) - rhs.num_ * int64_t(den_);
			int64_t d = den_ * int64_t(rhs.den_);
			ReduceFraction(n, d, INT_MAX);
			num_ = int(n);
			den_ = int(d);
			fix_signs();
		}
	}

	return *this;
}

const rational &rational::operator*=(const rational &rhs)
{
	if (*this == RATIONAL_MIN || *this == RATIONAL_MAX || rhs == RATIONAL_MIN ||
		rhs == RATIONAL_MAX) {
		*this = NaN;
	} else if (!isNaN()) {
		if (rhs.isNaN()) {
			*this = NaN;
		} else {
			int64_t n = num_ * int64_t(rhs.num_);
			int64_t d = den_ * int64_t(rhs.den_);
			ReduceFraction(n, d, INT_MAX);
			num_ = int(n);
			den_ = int(d);
			fix_signs();
		}
	}

	return *this;
}

const rational &rational::operator/=(const rational &rhs)
{
	if (*this == RATIONAL_MIN || *this == RATIONAL_MAX || rhs == RATIONAL_MIN ||
		rhs == RATIONAL_MAX) {
		*this = NaN;
	} else if (!isNaN()) {
		if (rhs.isNaN()) {
			*this = NaN;
		} else {
			int64_t n = num_ * int64_t(rhs.den_);
			int64_t d = den_ * int64_t(rhs.num_);
			ReduceFraction(n, d, INT_MAX);
			num_ = int(n);
			den_ = int(d);
			fix_signs();
		}
	}

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
	return CompareFractions(num_, den_, rhs.num_, rhs.den_) == -1;
}

bool rational::operator<=(const rational &rhs) const
{
	int cmp = CompareFractions(num_, den_, rhs.num_, rhs.den_);
	return cmp == 0 || cmp == -1;
}

bool rational::operator>(const rational &rhs) const
{
	return CompareFractions(num_, den_, rhs.num_, rhs.den_) == 1;
}

bool rational::operator>=(const rational &rhs) const
{
	int cmp = CompareFractions(num_, den_, rhs.num_, rhs.den_);
	return cmp == 0 || cmp == 1;
}

bool rational::operator==(const rational &rhs) const
{
	return CompareFractions(num_, den_, rhs.num_, rhs.den_) == 0;
}

bool rational::operator!=(const rational &rhs) const
{
	return !(*this == rhs);
}

}
