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

#ifndef FRAME_H
#define FRAME_H

#include <memory>
#include <QVector>

#include "common/rational.h"
#include "render/audioparams.h"
#include "render/pixelformat.h"

class Frame;
using FramePtr = std::shared_ptr<Frame>;

/**
 * @brief Video frame data or audio sample data from a Decoder
 *
 * Abstraction from AVFrame. Currently a simple AVFrame wrapper.
 *
 * This class does not support copying at this time.
 */
class Frame
{
public:
  /// Normal constructor
  Frame();

  static FramePtr Create();

  /**
   * @brief Get frame's width in pixels
   */
  const int& width();
  void set_width(const int& width);

  /**
   * @brief Get frame's height in pixels
   */
  const int& height();
  void set_height(const int& height);

  const AudioRenderingParams& audio_params();
  void set_audio_params(const AudioRenderingParams& params);

  const int &sample_count();
  void set_sample_count(const int &sample_count);

  /**
   * @brief Get frame's timestamp.
   *
   * This timestamp is always a rational that will equate to the time in seconds.
   */
  const rational& timestamp();
  void set_timestamp(const rational& timestamp);

  const int64_t& native_timestamp();
  void set_native_timestamp(const int64_t& timestamp);

  /**
   * @brief Get frame's format
   *
   * @return
   *
   * Currently this will either be an olive::PixelFormat (video) or an olive::SampleFormat (audio).
   */
  const olive::PixelFormat& format();
  void set_format(const olive::PixelFormat& format);

  /**
   * @brief Get the data buffer of this frame
   */
  uint8_t* data();

  /**
   * @brief Get the const data buffer of this frame
   */
  const uint8_t* const_data();

  /**
   * @brief Allocate memory buffer to store data based on parameters
   *
   * For video frames, the width(), height(), and format() must be set for this function to work.
   *
   * If a memory buffer has been previously allocated without destroying, this function will destroy it.
   */
  void allocate();

  /**
   * @brief Destroy a memory buffer allocated with allocate()
   */
  void destroy();

private:
  int width_;

  int height_;

  olive::PixelFormat format_;

  AudioRenderingParams audio_params_;

  int sample_count_;

  QVector<uint8_t> data_;

  rational timestamp_;

  int64_t native_timestamp_;

};

#endif // FRAME_H
