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
 */
class Frame
{
public:
  Frame();

  static FramePtr Create();

  /**
   * @brief Get frame's width in pixels
   */
  const int& width() const;
  void set_width(const int& width);

  /**
   * @brief Get frame's height in pixels
   */
  const int& height() const;
  void set_height(const int& height);

  const rational& sample_aspect_ratio() const;
  void set_sample_aspect_ratio(const rational& sample_aspect_ratio);

  const AudioRenderingParams& audio_params() const;
  void set_audio_params(const AudioRenderingParams& params);

  const int &sample_count() const;
  void set_sample_count(const int &sample_count);

  /**
   * @brief Get frame's timestamp.
   *
   * This timestamp is always a rational that will equate to the time in seconds.
   */
  const rational& timestamp() const;
  void set_timestamp(const rational& timestamp);

  /*const int64_t& native_timestamp();
  void set_native_timestamp(const int64_t& timestamp);*/

  /**
   * @brief Get frame's format
   *
   * @return
   *
   * Currently this will either be an olive::PixelFormat (video) or an olive::SampleFormat (audio).
   */
  const PixelFormat::Format& format() const;
  void set_format(const PixelFormat::Format& format);

  /**
   * @brief Returns a copy of the data in this frame as a QByteArray
   *
   * Will always do a deep copy. If you want to affect the data directly, use data() instead.
   */
  QByteArray ToByteArray() const;

  /**
   * @brief Get the data buffer of this frame
   */
  char* data();

  /**
   * @brief Get the const data buffer of this frame
   */
  const char* const_data() const;

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

  /**
   * @brief Returns the size of the array returned in data() in bytes
   *
   * Returns 0 if nothing is allocated.
   */
  int allocated_size() const;

private:
  int width_;

  int height_;

  PixelFormat::Format format_;

  AudioRenderingParams audio_params_;

  int sample_count_;

  QByteArray data_;

  rational timestamp_;

  rational sample_aspect_ratio_;

};

Q_DECLARE_METATYPE(FramePtr)

#endif // FRAME_H
