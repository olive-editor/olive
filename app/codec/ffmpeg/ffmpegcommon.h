#ifndef FFMPEGABSTRACTION_H
#define FFMPEGABSTRACTION_H

extern "C" {
#include <libavformat/avformat.h>
}

#include "audio/sampleformat.h"
#include "render/pixelformat.h"

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
  static SampleFormat GetNativeSampleFormat(const AVSampleFormat& smp_fmt);

  /**
   * @brief Returns an FFmpeg sample format type for a given native type
   */
  static AVSampleFormat GetFFmpegSampleFormat(const SampleFormat& smp_fmt);
};

#endif // FFMPEGABSTRACTION_H
