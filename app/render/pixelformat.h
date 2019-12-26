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

#include <OpenImageIO/imageio.h>
#include <QOpenGLExtraFunctions>
#include <QString>

class PixelFormat {
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

  /**
   * @brief A struct of information pertaining to each enum PixelFormat.
   *
   * Primarily this is a means of retrieving OpenGL texture information for different pixel formats/bit depths. Both
   * RAM and VRAM buffers will need a PixelFormat. To keep consistency between the OpenGL code and CPU code when using
   * a given PixelFormat, the PixelFormatInfo struct contains all necessary variables that you'll need to plug into
   * OpenGL.
   *
   * Use the static function PixelService::GetPixelFormatInfo to generate a PixelFormatInfo object.
   */
  struct Info {
    QString name;
    GLint internal_format;
    GLenum pixel_format;
    GLenum gl_pixel_type;
    int bytes_per_pixel;
    OIIO::TypeDesc oiio_desc;
  };
};

#endif // BITDEPTHS_H
