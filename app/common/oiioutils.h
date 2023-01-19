/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef OIIOUTILS_H
#define OIIOUTILS_H

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/typedesc.h>

#include "codec/frame.h"
#include "render/videoparams.h"

namespace olive {

class OIIOUtils {
public:
  static OIIO::TypeDesc::BASETYPE GetOIIOBaseTypeFromFormat(PixelFormat format)
  {
    switch (format) {
    case PixelFormat::U8:
      return OIIO::TypeDesc::UINT8;
    case PixelFormat::U16:
      return OIIO::TypeDesc::UINT16;
    case PixelFormat::F16:
      return OIIO::TypeDesc::HALF;
    case PixelFormat::F32:
      return OIIO::TypeDesc::FLOAT;
    case PixelFormat::INVALID:
    case PixelFormat::FORMAT_COUNT:
      break;
    }

    return OIIO::TypeDesc::UNKNOWN;
  }

  static void FrameToBuffer(const Frame *frame, OIIO::ImageBuf* buf);

  static void BufferToFrame(OIIO::ImageBuf* buf, Frame* frame);

  static PixelFormat GetFormatFromOIIOBasetype(OIIO::TypeDesc::BASETYPE type);

  static rational GetPixelAspectRatioFromOIIO(const OIIO::ImageSpec& spec);

};

}

#endif // OIIOUTILS_H
