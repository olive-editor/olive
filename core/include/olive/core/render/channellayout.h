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

#ifndef LIBOLIVECORE_CHANNELLAYOUT_H
#define LIBOLIVECORE_CHANNELLAYOUT_H

#include <stdint.h>

namespace olive::core
{

/**
 * @brief Channel layout masks used throughout Olive
 *
 * Audio channel layouts are represented as plain 64-bit masks. The values
 * deliberately mirror FFmpeg's AV_CH_LAYOUT_* constants so they can be passed
 * straight through to the FFmpeg bridge library; the bridge unit tests
 * static_assert each value against the real FFmpeg headers.
 */
inline constexpr uint64_t kChannelLayoutMono = 0x4;      ///< AV_CH_LAYOUT_MONO
inline constexpr uint64_t kChannelLayoutStereo = 0x3;    ///< AV_CH_LAYOUT_STEREO
inline constexpr uint64_t kChannelLayout2_1 = 0x103;     ///< AV_CH_LAYOUT_2_1
inline constexpr uint64_t kChannelLayout5Point1 = 0x60F; ///< AV_CH_LAYOUT_5POINT1
inline constexpr uint64_t kChannelLayout7Point1 = 0x63F; ///< AV_CH_LAYOUT_7POINT1

/**
 * @brief Number of channels in a layout mask (population count)
 */
inline int ChannelLayoutMaskChannelCount(uint64_t mask)
{
#if defined(__GNUC__) || defined(__clang__)
	return __builtin_popcountll(mask);
#else
	int count = 0;
	while (mask) {
		mask &= mask - 1;
		count++;
	}
	return count;
#endif
}

}

#endif // LIBOLIVECORE_CHANNELLAYOUT_H
