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

#include "common/rational.h"
#include "render/pixelformat.h"

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

  /// Copy constructor
  Frame(const Frame& f);

  /// Move constructor
  Frame(Frame&& f);

  /// Copy assignment operator
  Frame& operator=(const Frame& f);

  /// Move assignment operator
  Frame& operator=(Frame&& f);

  /// Destructor
  ~Frame();

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

  /**
   * @brief Get frame's timestamp.
   *
   * This timestamp is always a rational that will equate to the time in seconds.
   */
  const rational& timestamp();
  void set_timestamp(const rational& timestamp);

  /**
   * @brief Get frame's format
   *
   * @return
   *
   * Currently this will either be an olive::PixelFormat (video) or an olive::SampleFormat (audio).
   */
  const int& format();
  void set_format(const int& format);

  /**
   * @brief Get the data buffer of this frame
   */
  uint8_t* data();

  /**
   * @brief Get the const data buffer of this frame
   */
  const uint8_t* const_data();

  void allocate();

  void destroy();

private:
  int width_;

  int height_;

  int format_;

  uint8_t* data_;

  rational timestamp_;

};

using FramePtr = std::shared_ptr<Frame>;

#endif // FRAME_H
