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
#include "render/color.h"
#include "render/pixelformat.h"
#include "render/videoparams.h"

OLIVE_NAMESPACE_ENTER

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

  const VideoRenderingParams& video_params() const;
  void set_video_params(const VideoRenderingParams& params);

  const int& width() const;
  const int& height() const;
  const PixelFormat::Format& format() const;

  Color get_pixel(int x, int y) const;
  bool contains_pixel(int x, int y) const;

  const rational& sample_aspect_ratio() const;
  void set_sample_aspect_ratio(const rational& sample_aspect_ratio);

  /**
   * @brief Get frame's timestamp.
   *
   * This timestamp is always a rational that will equate to the time in seconds.
   */
  const rational& timestamp() const;
  void set_timestamp(const rational& timestamp);

  const int64_t& native_timestamp();
  void set_native_timestamp(const int64_t& timestamp);

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
   * @brief Return whether the frame is allocated or not
   */
  bool is_allocated() const;

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
  VideoRenderingParams params_;

  QByteArray data_;

  rational timestamp_;

  int64_t native_timestamp_;

  rational sample_aspect_ratio_;

};

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::FramePtr)

#endif // FRAME_H
