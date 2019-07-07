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

extern "C" {
#include <libavformat/avformat.h>
}

#include <memory>

#include "rational.h"

/**
 * @brief Abstraction from AVFrame. Currently a simple AVFrame wrapper.
 *
 * This class does not support copying at this time.
 */
class Frame
{
public:
  // Normal constructor
  Frame();

  // AVFrame constructor
  Frame(AVFrame* f);

  // Copy constructor
  Frame(const Frame& f);

  // Move constructor
  Frame(Frame&& f);

  // Copy assignment operator
  Frame& operator=(const Frame& f);

  // Move assignment operator
  Frame& operator=(Frame&& f);

  // Destructor
  ~Frame();

  /**
   * @brief Set frame child
   *
   * This class currently primarily functions as a wrapper for AVFrame for use outside of the Decoder classes.
   * The internal AVFrame is set here. This class will also take ownership of the AVFrame and automatically
   * clear it when deconstructed.
   *
   * @param f
   */
  void SetAVFrame(AVFrame* f, AVRational timebase);

  /**
   * @brief Get frame's width in pixels
   */
  const int& width();

  /**
   * @brief Get frame's height in pixels
   */
  const int& height();

  /**
   * @brief Get frame's timestamp.
   *
   * This timestamp is always a rational that will equate to the time in seconds.
   */
  const rational& timestamp();

  /**
   * @brief Get frame's format
   *
   * @return
   *
   * Currently this will either be an AVPixelFormat (video) or an AVSampleFormat (audio).
   */
  const int& format();

  /**
   * @brief Get the data buffer of this frame
   */
  uint8_t** data();

  /**
   * @brief Get the linesize information for this frame
   */
  int* linesize();

private:

  void FreeChild();

  AVFrame* frame_;
  rational timestamp_;
};

using FramePtr = std::shared_ptr<Frame>;

#endif // FRAME_H
