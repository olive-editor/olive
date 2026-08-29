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

#ifndef LIBOLIVECORE_FRACTIONUTILS_H
#define LIBOLIVECORE_FRACTIONUTILS_H

#include <stdint.h>

namespace olive::core
{

/**
 * @brief Rounding modes for RescaleRnd()
 *
 * Mirrors the FFmpeg AVRounding modes that this codebase used before the
 * FFmpeg dependency was removed from core.
 */
enum class FractionRounding {
	/**
	 * Round to the nearest value; halfway cases are rounded away from zero.
	 * Equivalent to FFmpeg's AV_ROUND_NEAR_INF.
	 */
	kNearInf,

	/**
	 * Round toward positive infinity. Equivalent to FFmpeg's AV_ROUND_UP.
	 */
	kUp
};

/**
 * @brief Reduce a fraction so that numerator and denominator fit within `max`
 *
 * Native re-implementation of FFmpeg's av_reduce(): divides out the greatest
 * common divisor and, if the values still do not fit within `max`, finds the
 * closest approximation using continued fractions.
 *
 * A zero denominator is preserved (with the numerator set to zero).
 */
void ReduceFraction(int64_t &num, int64_t &den, int64_t max);

/**
 * @brief Compare two fractions
 *
 * Native re-implementation of FFmpeg's av_cmp_q(): returns -1 if a < b,
 * 0 if a == b, 1 if a > b, and INT_MIN when the comparison is meaningless
 * (degenerate zero-denominator fractions).
 */
int CompareFractions(int an, int ad, int bn, int bd);

/**
 * @brief Rescale `a` by the fraction b/c: returns a * b / c
 *
 * Native re-implementation of FFmpeg's av_rescale_rnd(). The intermediate
 * product is computed with 128-bit arithmetic where available so that no
 * precision is lost for large timestamps.
 */
int64_t RescaleRnd(int64_t a, int64_t b, int64_t c, FractionRounding rnd);

}

#endif // LIBOLIVECORE_FRACTIONUTILS_H
