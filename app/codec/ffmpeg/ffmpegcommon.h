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

#ifndef FFMPEGABSTRACTION_H
#define FFMPEGABSTRACTION_H

extern "C" {
#include <libavformat/avformat.h>
}

#include "audio/sampleformat.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

class FFmpegCommon {
public:
  /**
   * @brief Returns an AVPixelFormat that can be used to convert a frame to a data type Olive supports with minimal data loss
   */
  static AVPixelFormat GetCompatiblePixelFormat(const AVPixelFormat& pix_fmt);

  /**
   * @brief Returns a native pixel format that can be used to convert from a native frame to an AVFrame with minimal data loss
   */
  static PixelFormat::Format GetCompatiblePixelFormat(const PixelFormat::Format& pix_fmt);

  /**
   * @brief Returns an FFmpeg pixel format for a given native pixel format
   */
  static AVPixelFormat GetFFmpegPixelFormat(const PixelFormat::Format& pix_fmt);

  /**
   * @brief Returns a native sample format type for a given AVSampleFormat
   */
  static SampleFormat::Format GetNativeSampleFormat(const AVSampleFormat& smp_fmt);

  /**
   * @brief Returns an FFmpeg sample format type for a given native type
   */
  static AVSampleFormat GetFFmpegSampleFormat(const SampleFormat::Format &smp_fmt);
};

OLIVE_NAMESPACE_EXIT

#endif // FFMPEGABSTRACTION_H
