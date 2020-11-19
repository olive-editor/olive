/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "oiioutils.h"

namespace olive {

void OIIOUtils::FrameToBuffer(const Frame* frame, OIIO::ImageBuf *buf)
{
  buf->set_pixels(OIIO::ROI(),
                  buf->spec().format,
                  frame->const_data(),
                  OIIO::AutoStride,
                  frame->linesize_bytes());
}

void OIIOUtils::BufferToFrame(OIIO::ImageBuf *buf, Frame* frame)
{
  buf->get_pixels(OIIO::ROI(),
                  buf->spec().format,
                  frame->data(),
                  OIIO::AutoStride,
                  frame->linesize_bytes());
}

rational OIIOUtils::GetPixelAspectRatioFromOIIO(const OIIO::ImageSpec &spec)
{
  return rational::fromDouble(spec.get_float_attribute("PixelAspectRatio", 1));
}

VideoParams::Format OIIOUtils::GetFormatFromOIIOBasetype(OIIO::TypeDesc::BASETYPE type)
{
  switch (type) {
  case OIIO::TypeDesc::UNKNOWN:
  case OIIO::TypeDesc::NONE:
    break;

  case OIIO::TypeDesc::INT8:
  case OIIO::TypeDesc::INT16:
  case OIIO::TypeDesc::INT32:
  case OIIO::TypeDesc::UINT32:
  case OIIO::TypeDesc::INT64:
  case OIIO::TypeDesc::UINT64:
  case OIIO::TypeDesc::STRING:
  case OIIO::TypeDesc::PTR:
  case OIIO::TypeDesc::LASTBASE:
  case OIIO::TypeDesc::DOUBLE:
    qDebug() << "Tried to use unknown OIIO base type";
    break;

  case OIIO::TypeDesc::UINT8:
    return VideoParams::kFormatUnsigned8;
  case OIIO::TypeDesc::UINT16:
    return VideoParams::kFormatUnsigned16;
  case OIIO::TypeDesc::HALF:
    return VideoParams::kFormatFloat16;
  case OIIO::TypeDesc::FLOAT:
    return VideoParams::kFormatFloat32;
  }

  return VideoParams::kFormatInvalid;
}

}
