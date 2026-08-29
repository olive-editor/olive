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

#include "util/fractionutils.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>

#include <limits>

namespace olive::core
{

namespace
{

int64_t i64_gcd(int64_t a, int64_t b)
{
	if (a < 0) {
		a = -a;
	}
	if (b < 0) {
		b = -b;
	}

	while (b) {
		int64_t t = a % b;
		a = b;
		b = t;
	}

	return a;
}

} // namespace

void ReduceFraction(int64_t &num, int64_t &den, int64_t max)
{
	if (den == 0) {
		num = 0;
		return;
	}

	int sign = (num < 0) != (den < 0);

	int64_t gcd = i64_gcd(num, den);
	if (gcd) {
		num = (num < 0 ? -num : num) / gcd;
		den = (den < 0 ? -den : den) / gcd;
	}

	if (num <= max && den <= max) {
		num = sign ? -num : num;
		return;
	}

	// Continued fraction approximation (ported from FFmpeg's av_reduce)
	int64_t a0n = 0, a0d = 1;
	int64_t a1n = 1, a1d = 0;

	while (den) {
		int64_t x = num / den;
		int64_t next_den = num - den * x;
		int64_t a2n = x * a1n + a0n;
		int64_t a2d = x * a1d + a0d;

		if (a2n > max || a2d > max) {
			if (a1n) {
				x = (max - a0n) / a1n;
			}
			if (a1d && (max - a0d) / a1d < x) {
				x = (max - a0d) / a1d;
			}

			if (den * (2 * x * a1d + a0d) > num * a1d) {
				a1n = x * a1n + a0n;
				a1d = x * a1d + a0d;
			}
			break;
		}

		a0n = a1n;
		a0d = a1d;
		a1n = a2n;
		a1d = a2d;
		num = den;
		den = next_den;
	}

	num = sign ? -a1n : a1n;
	den = a1d;
}

int CompareFractions(int an, int ad, int bn, int bd)
{
	const int64_t tmp = an * int64_t(bd) - bn * int64_t(ad);

	if (tmp) {
		return int(((tmp ^ ad ^ bd) >> 63) | 1);
	} else if (bd && ad) {
		return 0;
	} else if (an && bn) {
		return (an >> 31) - (bn >> 31);
	}

	return INT_MIN;
}

int64_t RescaleRnd(int64_t a, int64_t b, int64_t c, FractionRounding rnd)
{
	// Normalize so that the divisor is positive; the sign is carried by the
	// dividend instead.
	if (c < 0) {
		c = -c;
		b = -b;
	}

#if defined(__SIZEOF_INT128__)
	// 128-bit intermediate: exact for all 64-bit inputs
	__int128 r = __int128(a) * __int128(b);
	bool negative = r < 0;
	unsigned __int128 ur = negative ? -r : r;
	unsigned __int128 uc = static_cast<unsigned __int128>(c);

	unsigned __int128 q;
	if (rnd == FractionRounding::kNearInf) {
		// Round to nearest, ties away from zero
		q = (ur + uc / 2) / uc;
	} else {
		// Round toward positive infinity
		if (!negative) {
			q = (ur + uc - 1) / uc;
		} else {
			q = ur / uc;
		}
	}

	int64_t res = int64_t(q);
	return negative ? -res : res;
#else
	// Portable fallback: cross-reduce to keep the intermediate product in
	// 64-bit range, then divide with the requested rounding.
	int64_t g = i64_gcd(b, c);
	if (g) {
		b /= g;
		c /= g;
	}
	g = i64_gcd(a, c);
	if (g) {
		a /= g;
		c /= g;
	}

	bool negative = (a < 0) != (b < 0);
	int64_t ua = a < 0 ? -a : a;
	int64_t ub = b < 0 ? -b : b;

	int64_t q;
	if (ua != 0 && ub > std::numeric_limits<int64_t>::max() / ua) {
		// Extremely unlikely: the product still overflows int64, fall back
		// to floating point (may lose precision for huge values).
		long double v = (long double)a * (long double)b / (long double)c;
		if (rnd == FractionRounding::kNearInf) {
			v = v >= 0 ? floorl(v + 0.5L) : ceill(v - 0.5L);
		} else {
			v = ceill(v);
		}
		return int64_t(v);
	}

	int64_t u = ua * ub;
	if (rnd == FractionRounding::kNearInf) {
		q = (u + c / 2) / c;
	} else {
		if (!negative) {
			q = (u + c - 1) / c;
		} else {
			q = u / c;
		}
	}

	return negative ? -q : q;
#endif
}

}
