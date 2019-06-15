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

#include "pixelformats.h"

#include <QCoreApplication>

namespace olive {

QVector<PixelFormatInfo> pixel_formats;

void InitializePixelFormats() {

  pixel_formats.resize(PIX_FMT_COUNT);

  pixel_formats[PIX_FMT_RGBA8].name = QCoreApplication::translate("bitdepths", "8-bit");
  pixel_formats[PIX_FMT_RGBA8].internal_format = GL_RGBA8;
  pixel_formats[PIX_FMT_RGBA8].pixel_format = GL_RGBA;
  pixel_formats[PIX_FMT_RGBA8].pixel_type = GL_UNSIGNED_BYTE;
  pixel_formats[PIX_FMT_RGBA8].bytes_per_pixel = 4;

  pixel_formats[PIX_FMT_RGBA16].name = QCoreApplication::translate("bitdepths", "16-bit Integer");
  pixel_formats[PIX_FMT_RGBA16].internal_format = GL_RGBA16;
  pixel_formats[PIX_FMT_RGBA16].pixel_format = GL_RGBA;
  pixel_formats[PIX_FMT_RGBA16].pixel_type = GL_UNSIGNED_SHORT;
  pixel_formats[PIX_FMT_RGBA16].bytes_per_pixel = 8;

  pixel_formats[PIX_FMT_RGBA16F].name = QCoreApplication::translate("bitdepths", "Half-Float (16-bit)");
  pixel_formats[PIX_FMT_RGBA16F].internal_format = GL_RGBA16F;
  pixel_formats[PIX_FMT_RGBA16F].pixel_format = GL_RGBA;
  pixel_formats[PIX_FMT_RGBA16F].pixel_type = GL_HALF_FLOAT;
  pixel_formats[PIX_FMT_RGBA16F].bytes_per_pixel = 8;

  pixel_formats[PIX_FMT_RGBA32F].name = QCoreApplication::translate("bitdepths", "Full-Float (32-bit)");
  pixel_formats[PIX_FMT_RGBA32F].internal_format = GL_RGBA32F;
  pixel_formats[PIX_FMT_RGBA32F].pixel_format = GL_RGBA;
  pixel_formats[PIX_FMT_RGBA32F].pixel_type = GL_FLOAT;
  pixel_formats[PIX_FMT_RGBA32F].bytes_per_pixel = 16;

}

}

