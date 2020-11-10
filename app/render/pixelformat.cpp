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

#include "pixelformat.h"

#include "OpenImageIO/imagebuf.h"
#include <QCoreApplication>
#include <QDebug>
#include <QFloat16>

#include "codec/oiio/oiiocommon.h"
#include "common/define.h"
#include "core.h"

OLIVE_NAMESPACE_ENTER

bool PixelFormat::FormatIsFloat(const PixelFormat::Format &format)
{
  switch (format) {
  case PixelFormat::PIX_FMT_RGBA16F:
  case PixelFormat::PIX_FMT_RGBA32F:
    return true;

  case PixelFormat::PIX_FMT_RGBA8:
  case PixelFormat::PIX_FMT_RGBA16U:
  case PixelFormat::PIX_FMT_INVALID:
  case PixelFormat::PIX_FMT_COUNT:
    break;
  }

  return false;
}

OIIO::TypeDesc::BASETYPE PixelFormat::GetOIIOTypeDesc(const PixelFormat::Format &format)
{
  switch (format) {
  case PixelFormat::PIX_FMT_RGBA8:
    return OIIO::TypeDesc::UINT8;
  case PixelFormat::PIX_FMT_RGBA16U:
    return OIIO::TypeDesc::UINT16;
  case PixelFormat::PIX_FMT_RGBA16F:
    return OIIO::TypeDesc::HALF;
  case PixelFormat::PIX_FMT_RGBA32F:
    return OIIO::TypeDesc::FLOAT;
  case PixelFormat::PIX_FMT_INVALID:
  case PixelFormat::PIX_FMT_COUNT:
    break;
  }

  return OIIO::TypeDesc::UNKNOWN;
}

QString PixelFormat::GetName(const PixelFormat::Format &format)
{
  switch (format) {
  case PixelFormat::PIX_FMT_RGBA8:
    return tr("8-bit");
  case PixelFormat::PIX_FMT_RGBA16U:
    return tr("16-bit Integer");
  case PixelFormat::PIX_FMT_RGBA16F:
    return tr("Half-Float (16-bit)");
  case PixelFormat::PIX_FMT_RGBA32F:
    return tr("Full-Float (32-bit)");
  case PixelFormat::PIX_FMT_INVALID:
  case PixelFormat::PIX_FMT_COUNT:
    break;
  }

  return tr("Unknown (%1)").arg(format);
}

PixelFormat* PixelFormat::instance_ = nullptr;

void PixelFormat::CreateInstance()
{
  instance_ = new PixelFormat();
}

void PixelFormat::DestroyInstance()
{
  delete instance_;
}

PixelFormat *PixelFormat::instance()
{
  return instance_;
}

PixelFormat::Format PixelFormat::GetConfiguredFormatForMode(RenderMode::Mode mode)
{
  return static_cast<PixelFormat::Format>(Core::GetPreferenceForRenderMode(mode, QStringLiteral("PixelFormat")).toInt());
}

void PixelFormat::SetConfiguredFormatForMode(RenderMode::Mode mode, PixelFormat::Format format)
{
  if (format != GetConfiguredFormatForMode(mode)) {
    Core::SetPreferenceForRenderMode(mode, QStringLiteral("PixelFormat"), format);

    emit FormatChanged();
  }
}

PixelFormat::Format PixelFormat::OIIOFormatToOliveFormat(OIIO::TypeDesc desc)
{
  if (desc == OIIO::TypeDesc::UINT8) {
    return PixelFormat::PIX_FMT_RGBA8;
  } else if (desc == OIIO::TypeDesc::UINT16) {
    return PixelFormat::PIX_FMT_RGBA16U;
  } else if (desc == OIIO::TypeDesc::HALF) {
    return PixelFormat::PIX_FMT_RGBA16F;
  } else if (desc == OIIO::TypeDesc::FLOAT) {
    return PixelFormat::PIX_FMT_RGBA32F;
  }

  return PixelFormat::PIX_FMT_INVALID;
}

int PixelFormat::GetBufferSize(const PixelFormat::Format &format, const int &width, const int &height)
{
  return BytesPerPixel(format) * width * height;
}

int PixelFormat::BytesPerPixel(const PixelFormat::Format &format)
{
  return BytesPerChannel(format) * kRGBAChannels;
}

int PixelFormat::BytesPerChannel(const PixelFormat::Format &format)
{
  switch (format) {
  case PixelFormat::PIX_FMT_RGBA8:
    return 1;
  case PixelFormat::PIX_FMT_RGBA16U:
  case PixelFormat::PIX_FMT_RGBA16F:
    return 2;
  case PixelFormat::PIX_FMT_RGBA32F:
    return 4;
  case PixelFormat::PIX_FMT_INVALID:
  case PixelFormat::PIX_FMT_COUNT:
    break;
  }

  qFatal("Invalid pixel format requested");

  // qFatal will abort so we won't get here, but this suppresses compiler warnings
  return 0;
}

FramePtr PixelFormat::ConvertPixelFormat(FramePtr frame, const PixelFormat::Format &dest_format)
{
  if (frame->format() == dest_format) {
    return frame;
  }

  // Create a destination frame with the same parameters
  FramePtr converted = Frame::Create();
  converted->set_video_params(VideoParams(frame->video_params().width(),
                                          frame->video_params().height(),
                                          dest_format));
  converted->set_timestamp(frame->timestamp());
  converted->allocate();

  // Do the conversion through OIIO - create a buffer for the source image
  OIIO::ImageBuf src(OIIO::ImageSpec(frame->width(),
                                     frame->height(),
                                     kRGBAChannels,
                                     GetOIIOTypeDesc(frame->format())));

  // Set the pixels (this is necessary as opposed to an OIIO buffer wrapper since Frame has
  // linesizes)
  OIIOCommon::FrameToBuffer(frame, &src);

  // Create a destination OIIO buffer with our destination format
  OIIO::ImageBuf dst(OIIO::ImageSpec(converted->width(),
                                     converted->height(),
                                     kRGBAChannels,
                                     GetOIIOTypeDesc(converted->format())));

  if (dst.copy_pixels(src)) {

    // Convert our buffer back to a frame
    OIIOCommon::BufferToFrame(&dst, converted);

    return converted;
  } else {
    return nullptr;
  }
}

OLIVE_NAMESPACE_EXIT
