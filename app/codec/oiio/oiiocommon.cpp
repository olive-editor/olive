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

#include "oiiocommon.h"

OLIVE_NAMESPACE_ENTER

void OIIOCommon::FrameToBuffer(FramePtr frame, OIIO::ImageBuf *buf)
{
#if OIIO_VERSION < 20112
  //
  // Workaround for OIIO bug that ignores destination stride in versions OLDER than 2.1.12
  //
  // See more: https://github.com/OpenImageIO/oiio/pull/2487
  //
  int width_in_bytes = frame->width() * PixelFormat::BytesPerPixel(frame->format());

  for (int i=0;i<buf->spec().height;i++) {
    memcpy(
#if OIIO_VERSION < 10903
          reinterpret_cast<char*>(buf->localpixels()) + i * width_in_bytes,
#else
          reinterpret_cast<char*>(buf->localpixels()) + i * buf->scanline_stride(),
#endif
          frame->data() + i * frame->linesize_bytes(),
          width_in_bytes);
  }
#else
  buf->set_pixels(OIIO::ROI(),
                  buf->spec().format,
                  frame->data(),
                  OIIO::AutoStride,
                  frame->linesize_bytes());
#endif
}

void OIIOCommon::BufferToFrame(OIIO::ImageBuf *buf, FramePtr frame)
{
#if OIIO_VERSION < 20112
  //
  // Workaround for OIIO bug that ignores destination stride in versions OLDER than 2.1.12
  //
  // See more: https://github.com/OpenImageIO/oiio/pull/2487
  //
  int width_in_bytes = frame->width() * PixelFormat::BytesPerPixel(frame->format());

  for (int i=0;i<buf->spec().height;i++) {
    memcpy(frame->data() + i * frame->linesize_bytes(),
#if OIIO_VERSION < 10903
           reinterpret_cast<const char*>(buf->localpixels()) + i * width_in_bytes,
#else
           reinterpret_cast<const char*>(buf->localpixels()) + i * buf->scanline_stride(),
#endif
           width_in_bytes);
  }
#else
  buf->get_pixels(OIIO::ROI(),
                  buf->spec().format,
                  frame->data(),
                  OIIO::AutoStride,
                  frame->linesize_bytes());
#endif
}

PixelFormat::Format OIIOCommon::GetFormatFromOIIOBasetype(const OIIO::ImageSpec& spec)
{
  if (spec.format == OIIO::TypeDesc::UINT8) {
    return PixelFormat::PIX_FMT_RGBA8;
  } else if (spec.format == OIIO::TypeDesc::UINT16) {
    return PixelFormat::PIX_FMT_RGBA16U;
  } else if (spec.format == OIIO::TypeDesc::HALF) {
    return PixelFormat::PIX_FMT_RGBA16F;
  } else if (spec.format == OIIO::TypeDesc::FLOAT) {
    return PixelFormat::PIX_FMT_RGBA32F;
  } else {
    return PixelFormat::PIX_FMT_INVALID;
  }
}

rational OIIOCommon::GetPixelAspectRatioFromOIIO(const OIIO::ImageSpec &spec)
{
  return rational::fromDouble(spec.get_float_attribute("PixelAspectRatio", 1));
}

OLIVE_NAMESPACE_EXIT
