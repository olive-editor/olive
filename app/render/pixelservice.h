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

#ifndef PIXELSERVICE_H
#define PIXELSERVICE_H

#include <QString>

#include "codec/frame.h"
#include "pixelformat.h"

class PixelService : public QObject {
public:

  PixelService();

  /**
   * @brief Return a PixelFormatInfo containing information for a certain format
   *
   * \see PixelFormatInfo
   */
  static PixelFormat::Info GetPixelFormatInfo(const PixelFormat::Format& format);

  /**
   * @brief Returns the minimum buffer size (in bytes) necessary for a given format, width, and height.
   *
   * @param format
   *
   * The format of the data the buffer should contain. Must be a member of the olive::PixelFormat enum.
   *
   * @param width
   *
   * The width (in pixels) of the buffer.
   *
   * @param height
   *
   * The height (in pixels) of the buffer.
   */
  static int GetBufferSize(const PixelFormat::Format &format, const int& width, const int& height);

  /**
   * @brief Returns the number of bytes per pixel for a certain format
   *
   * Different formats use different sizes of data for pixels. Use this function to determine how many bytes a pixel
   * requires for a certain format. The number of bytes will always be a multiple of 4 since all formats use RGBA and
   * are at least 1 bpc.
   */
  static int BytesPerPixel(const PixelFormat::Format &format);

  /**
   * @brief Returns the number of bytes per channel for a certain format
   */
  static int BytesPerChannel(const PixelFormat::Format& format);

  /**
   * @brief Convert a frame to a pixel format
   *
   * If the frame's pixel format == the destination format, this just returns `frame`.
   */
  static FramePtr ConvertPixelFormat(FramePtr frame, const PixelFormat::Format &dest_format);

  /**
   * @brief Convert an RGB image to an RGBA image
   */
  static void ConvertRGBtoRGBA(FramePtr frame);

};

#endif // PIXELSERVICE_H
