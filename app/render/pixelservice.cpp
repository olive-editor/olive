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

#include "pixelservice.h"

#include <QCoreApplication>

const int kRGBAChannels = 4;

PixelService::PixelService()
{
}

PixelFormatInfo PixelService::GetPixelFormatInfo(const olive::PixelFormat &format)
{
  PixelFormatInfo info;

  switch (format) {
  case olive::PIX_FMT_RGBA8:
    info.name = tr("8-bit");
    info.internal_format = GL_RGBA8;
    info.pixel_type = GL_UNSIGNED_BYTE;
    break;
  case olive::PIX_FMT_RGBA16:
    info.name = tr("16-bit Integer");
    info.internal_format = GL_RGBA16;
    info.pixel_type = GL_UNSIGNED_SHORT;
    break;
  case olive::PIX_FMT_RGBA16F:
    info.name = tr("Half-Float (16-bit)");
    info.internal_format = GL_RGBA16F;
    info.pixel_type = GL_HALF_FLOAT;
    break;
  case olive::PIX_FMT_RGBA32F:
    info.name = tr("Full-Float (32-bit)");
    info.internal_format = GL_RGBA32F;
    info.pixel_type = GL_FLOAT;
    break;
  default:
    qFatal("Invalid pixel format requested");
  }

  info.pixel_format = GL_RGBA;
  info.bytes_per_pixel = BytesPerPixel(format);

  return info;
}

int PixelService::GetBufferSize(const olive::PixelFormat &format, const int &width, const int &height)
{
  return BytesPerPixel(format) * width * height;
}

int PixelService::BytesPerPixel(const olive::PixelFormat &format)
{
  return BytesPerChannel(format) * kRGBAChannels;
}

int PixelService::BytesPerChannel(const olive::PixelFormat &format)
{
  switch (format) {
  case olive::PIX_FMT_RGBA8:
    return 1;
  case olive::PIX_FMT_RGBA16:
  case olive::PIX_FMT_RGBA16F:
    return 2;
  case olive::PIX_FMT_RGBA32F:
    return 4;
  default:
    qFatal("Invalid pixel format requested");
  }
}
