/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <memory>

#include "render/pixelformat.h"
#include "render/sampleformat.h"

namespace olive {

class FFmpegUtils {
public:
  /**
   * @brief Returns an AVPixelFormat that can be used to convert a frame to a data type Olive supports with minimal data loss
   */
  static AVPixelFormat GetCompatiblePixelFormat(const AVPixelFormat& pix_fmt, PixelFormat maximum = PixelFormat::INVALID);

  /**
   * @brief Returns a native pixel format that can be used to convert from a native frame to an AVFrame with minimal data loss
   */
  static PixelFormat GetCompatiblePixelFormat(const PixelFormat& pix_fmt);

  /**
   * @brief Returns an FFmpeg pixel format for a given native pixel format
   */
  static AVPixelFormat GetFFmpegPixelFormat(const PixelFormat& pix_fmt, int channel_layout);

  /**
   * @brief Returns a native sample format type for a given AVSampleFormat
   */
  static SampleFormat GetNativeSampleFormat(const AVSampleFormat& smp_fmt);

  /**
   * @brief Returns an FFmpeg sample format type for a given native type
   */
  static AVSampleFormat GetFFmpegSampleFormat(const SampleFormat &smp_fmt);

  /**
   * @brief Returns an SWS_CS_* macro from an AVColorSpace enum member
   *
   * Why aren't these the same thing anyway? And for that matter, why doesn't FFmpeg provide a
   * convenience function to do this conversion for us? Who knows, but here we are.
   */
  static int GetSwsColorspaceFromAVColorSpace(AVColorSpace cs);

  /**
   * @brief Convert "JPEG"/full-range colorspace to its regular counterpart
   *
   * "JPEG "spaces are deprecated in favor of the regular space and setting `color_range`. For the
   * time being, FFmpeg still uses these JPEG spaces, so for simplicity (since we *are* color_range
   * aware), we use this function.
   */
  static AVPixelFormat ConvertJPEGSpaceToRegularSpace(AVPixelFormat f);
};

using AVFramePtr = std::shared_ptr<AVFrame>;
inline AVFramePtr CreateAVFramePtr(AVFrame *f)
{
  return std::shared_ptr<AVFrame>(f, [](AVFrame *g){ av_frame_free(&g); });
}
inline AVFramePtr CreateAVFramePtr()
{
  return CreateAVFramePtr(av_frame_alloc());
}

}

#endif // FFMPEGABSTRACTION_H
