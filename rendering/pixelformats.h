/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include <QString>
#include <QVector>
#include <QOpenGLExtraFunctions>

namespace olive {

struct PixelFormatInfo {
  QString name;
  GLint internal_format;
  GLenum pixel_format;
  GLenum pixel_type;
  int bytes_per_pixel;
};

/**
 * @brief The PixelFormat enum
 *
 * Olive's internal supported pixel formats. With the exception of OLIVE_PIX_FMT_COUNT, these must all
 * be defined in InitializePixelFormats().
 */
enum PixelFormat {
  PIX_FMT_RGBA8,
  PIX_FMT_RGBA16,
  PIX_FMT_RGBA16F,
  PIX_FMT_RGBA32F,
  PIX_FMT_COUNT
};

extern QVector<PixelFormatInfo> pixel_formats;

void InitializePixelFormats();

}

#endif // BITDEPTHS_H
