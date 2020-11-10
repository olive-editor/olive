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

#ifndef BITDEPTHS_H
#define BITDEPTHS_H

#include <memory>
#include <OpenImageIO/imageio.h>
#include <QOpenGLExtraFunctions>
#include <QString>

#include "render/rendermodes.h"

OLIVE_NAMESPACE_ENTER

class Frame;
using FramePtr = std::shared_ptr<Frame>;

class PixelFormat : public QObject
{
  Q_OBJECT
public:
  /**
   * @brief Olive's internal supported pixel formats.
   */
  enum Format {
    PIX_FMT_INVALID = -1,

    PIX_FMT_RGBA8,
    PIX_FMT_RGBA16U,
    PIX_FMT_RGBA16F,
    PIX_FMT_RGBA32F,

    PIX_FMT_COUNT
  };

  static void CreateInstance();
  static void DestroyInstance();
  static PixelFormat* instance();

  /**
   * @brief Returns the configured pixel format for a given mode
   */
  Format GetConfiguredFormatForMode(RenderMode::Mode mode);
  void SetConfiguredFormatForMode(RenderMode::Mode mode, PixelFormat::Format format);

  static Format OIIOFormatToOliveFormat(OIIO::TypeDesc desc);

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
  static int GetBufferSize(const Format &format, const int& width, const int& height);

  /**
   * @brief Returns the number of bytes per pixel for a certain format
   *
   * Different formats use different sizes of data for pixels. Use this function to determine how many bytes a pixel
   * requires for a certain format. The number of bytes will always be a multiple of 4 since all formats use RGBA and
   * are at least 1 bpc.
   */
  static int BytesPerPixel(const Format &format);

  /**
   * @brief Returns the number of bytes per channel for a certain format
   */
  static int BytesPerChannel(const Format& format);

  /**
   * @brief Convert a frame to a pixel format
   *
   * If the frame's pixel format == the destination format, this just returns `frame`.
   */
  static FramePtr ConvertPixelFormat(FramePtr frame, const Format &dest_format);

  /**
   * @brief Simple convenience function returning whether a pixel format is float-based or integer-based
   */
  static bool FormatIsFloat(const Format& format);

  /**
   * @brief Get corresponding OpenImageIO TypeDesc for a given pixel format
   */
  static OIIO::TypeDesc::BASETYPE GetOIIOTypeDesc(const Format& format);

  /**
   * @brief Get format name
   */
  static QString GetName(const Format& format);

signals:
  void FormatChanged();

private:
  PixelFormat() = default;

  static PixelFormat* instance_;

};

OLIVE_NAMESPACE_EXIT

#endif // BITDEPTHS_H
