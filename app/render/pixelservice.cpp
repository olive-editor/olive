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
#include <QDebug>
#include <QFloat16>

#include "common/define.h"

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
    info.oiio_desc = OIIO::TypeDesc::UINT8;
    break;
  case olive::PIX_FMT_RGBA16:
    info.name = tr("16-bit Integer");
    info.internal_format = GL_RGBA16;
    info.pixel_type = GL_UNSIGNED_SHORT;
    info.oiio_desc = OIIO::TypeDesc::UINT16;
    break;
  case olive::PIX_FMT_RGBA16F:
    info.name = tr("Half-Float (16-bit)");
    info.internal_format = GL_RGBA16F;
    info.pixel_type = GL_HALF_FLOAT;
    info.oiio_desc = OIIO::TypeDesc::HALF;
    break;
  case olive::PIX_FMT_RGBA32F:
    info.name = tr("Full-Float (32-bit)");
    info.internal_format = GL_RGBA32F;
    info.pixel_type = GL_FLOAT;
    info.oiio_desc = OIIO::TypeDesc::FLOAT;
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

FramePtr PixelService::ConvertPixelFormat(FramePtr frame, const olive::PixelFormat &dest_format)
{
  if (frame->format() == dest_format) {
    return frame;
  }

  // FIXME: It'd be nice if this was multithreaded soon

  FramePtr converted = std::make_shared<Frame>();

  // Copy parameters
  converted->set_width(frame->width());
  converted->set_height(frame->height());
  converted->set_timestamp(frame->timestamp());
  converted->set_format(dest_format);
  converted->allocate();

  int pix_count = frame->width() * frame->height() * kRGBAChannels;

  bool valid = true;

  switch (static_cast<olive::PixelFormat>(frame->format())) {
  case olive::PIX_FMT_RGBA8:
  {
    uint8_t* source = frame->data();

    switch (dest_format) {
    case olive::PIX_FMT_RGBA16: // 8-bit Integer -> 16-bit Integer
    {
      uint16_t* destination = reinterpret_cast<uint16_t*>(converted->data());
      for (int i=0;i<pix_count;i++) {
        destination[i] = static_cast<uint16_t>(source[i] * 257);
      }
      break;
    }
    case olive::PIX_FMT_RGBA16F: // 8-bit Integer -> 16-bit Float
    {
      qfloat16* destination = reinterpret_cast<qfloat16*>(converted->data());
      for (int i=0;i<pix_count;i++) {
        destination[i] = source[i] / 255.0f;
      }
      break;
    }
    case olive::PIX_FMT_RGBA32F: // 8-bit Integer -> 32-bit Float
    {
      float* destination = reinterpret_cast<float*>(converted->data());
      for (int i=0;i<pix_count;i++) {
        destination[i] = source[i] / 255.0f;
      }
      break;
    }
    case olive::PIX_FMT_INVALID:
    case olive::PIX_FMT_RGBA8:
    case olive::PIX_FMT_COUNT:
      valid = false;
    }
    break;
  }
  case olive::PIX_FMT_RGBA16:
  {
    uint16_t* source = reinterpret_cast<uint16_t*>(frame->data());

    switch (dest_format) {
    case olive::PIX_FMT_RGBA8: // 16-bit Integer -> 8-bit Integer
    {
      uint8_t* destination = converted->data();
      for (int i=0;i<pix_count;i++) {
        destination[i] = static_cast<uint8_t>(source[i] / 257);
      }
      break;
    }
    case olive::PIX_FMT_RGBA16F: // 16-bit Integer -> 16-bit Float
    {
      qfloat16* destination = reinterpret_cast<qfloat16*>(converted->data());
      for (int i=0;i<pix_count;i++) {
        destination[i] = source[i] / 65535.0f;
      }
      break;
    }
    case olive::PIX_FMT_RGBA32F: // 16-bit Integer -> 32-bit Float
    {
      float* destination = reinterpret_cast<float*>(converted->data());
      for (int i=0;i<pix_count;i++) {
        destination[i] = source[i] / 65535.0f;
      }
      break;
    }
    case olive::PIX_FMT_INVALID:
    case olive::PIX_FMT_RGBA16:
    case olive::PIX_FMT_COUNT:
      valid = false;
    }
    break;
  }
  case olive::PIX_FMT_RGBA16F:
  {
    qfloat16* source = reinterpret_cast<qfloat16*>(frame->data());

    switch (dest_format) {
    case olive::PIX_FMT_RGBA8: // 16-bit Float -> 8-bit Integer
    {
      uint8_t* destination = converted->data();
      for (int i=0;i<pix_count;i++) {
        destination[i] = static_cast<uint8_t>(source[i] * 255.0f);
      }
      break;
    }
    case olive::PIX_FMT_RGBA16: // 16-bit Float -> 16-bit Integer
    {
      uint16_t* destination = reinterpret_cast<uint16_t*>(converted->data());
      for (int i=0;i<pix_count;i++) {
        destination[i] = static_cast<uint16_t>(source[i] * 65535.0f);
      }
      break;
    }
    case olive::PIX_FMT_RGBA32F: // 16-bit Float -> 32-bit Float
    {
      float* destination = reinterpret_cast<float*>(converted->data());
      for (int i=0;i<pix_count;i++) {
        destination[i] = source[i];
      }
      break;
    }
    case olive::PIX_FMT_INVALID:
    case olive::PIX_FMT_RGBA16F:
    case olive::PIX_FMT_COUNT:
      valid = false;
    }
    break;
  }
  case olive::PIX_FMT_RGBA32F:
  {
    float* source = reinterpret_cast<float*>(frame->data());

    switch (dest_format) {
    case olive::PIX_FMT_RGBA8: // 32-bit Float -> 8-bit Integer
    {
      uint8_t* destination = converted->data();
      for (int i=0;i<pix_count;i++) {
        destination[i] = static_cast<uint8_t>(source[i] * 255.0f);
      }
      break;
    }
    case olive::PIX_FMT_RGBA16: // 32-bit Float -> 16-bit Integer
    {
      uint16_t* destination = reinterpret_cast<uint16_t*>(converted->data());
      for (int i=0;i<pix_count;i++) {
        destination[i] = static_cast<uint16_t>(source[i] * 255.0f);
      }
      break;
    }
    case olive::PIX_FMT_RGBA16F: // 32-bit Float -> 16-bit Float
    {
      qfloat16* destination = reinterpret_cast<qfloat16*>(converted->data());
      for (int i=0;i<pix_count;i++) {
        destination[i] = source[i];
      }
      break;
    }
    case olive::PIX_FMT_INVALID:
    case olive::PIX_FMT_RGBA32F:
    case olive::PIX_FMT_COUNT:
      valid = false;
    }
    break;
  }
  case olive::PIX_FMT_INVALID:
  case olive::PIX_FMT_COUNT:
    valid = false;
  }

  if (valid) {
    return converted;
  }

  qWarning() << tr("Invalid parameters called for pixel format conversion");
  return nullptr;
}
