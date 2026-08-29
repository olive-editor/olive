/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team
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

#ifndef FFMPEGABSTRACTION_H
#define FFMPEGABSTRACTION_H

#include <ffmpeg_bridge/ffmpeg_bridge.h>

#include <olive/core/core.h>

#include "render/videoparams.h"

namespace olive
{

using namespace core;

/**
 * @brief C++ adapter mapping Olive's native enums to bridge pixel/sample
 * formats
 *
 * All "FFmpeg" formats here are actually the opaque FBPixelFormat /
 * FBSampleFormat constants of the ffmpeg_bridge library; no FFmpeg header
 * or structure is ever seen by the editor.
 */
class FFmpegUtils {
public:
	/**
   * @brief Returns a bridge pixel format that can be used to convert a frame to a data type Olive supports with minimal data loss
   *
   * Named distinctly from the native PixelFormat overload below: with both
   * taking a single argument, an unscoped enum argument would silently
   * prefer an int overload over the PixelFormat one.
   */
	static int GetCompatibleBridgePixelFormat(
		int pix_fmt, PixelFormat maximum = PixelFormat::INVALID);

	/**
   * @brief Returns a native pixel format that can be used to convert from a native frame to a bridge frame with minimal data loss
   */
	static PixelFormat GetCompatiblePixelFormat(const PixelFormat &pix_fmt);

	/**
   * @brief Returns a bridge pixel format for a given native pixel format
   */
	static int GetFFmpegPixelFormat(const PixelFormat &pix_fmt,
									int channel_layout);

	/**
   * @brief Returns a native sample format type for a given bridge sample format
   */
	static SampleFormat GetNativeSampleFormat(int smp_fmt);

	/**
   * @brief Returns a bridge sample format type for a given native type
   */
	static int GetFFmpegSampleFormat(const SampleFormat &smp_fmt);

	/**
   * @brief Convert "JPEG"/full-range colorspace to its regular counterpart
   *
   * "JPEG "spaces are deprecated in favor of the regular space and setting `color_range`. For the
   * time being, FFmpeg still uses these JPEG spaces, so for simplicity (since we *are* color_range
   * aware), we use this function.
   */
	static int ConvertJPEGSpaceToRegularSpace(int f);
};

}

#endif // FFMPEGABSTRACTION_H
