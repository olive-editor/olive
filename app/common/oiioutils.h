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

#ifndef OIIOUTILS_H
#define OIIOUTILS_H

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/typedesc.h>

#include "codec/frame.h"
#include "render/videoparams.h"

OLIVE_NAMESPACE_ENTER

class OIIOUtils {
public:
  static OIIO::TypeDesc::BASETYPE GetOIIOBaseTypeFromFormat(VideoParams::Format format)
  {
    switch (format) {
    case VideoParams::kFormatUnsigned8:
      return OIIO::TypeDesc::UINT8;
    case VideoParams::kFormatUnsigned16:
      return OIIO::TypeDesc::UINT16;
    case VideoParams::kFormatFloat16:
      return OIIO::TypeDesc::HALF;
    case VideoParams::kFormatFloat32:
      return OIIO::TypeDesc::FLOAT;
    case VideoParams::kFormatInvalid:
    case VideoParams::kFormatCount:
      break;
    }

    return OIIO::TypeDesc::UNKNOWN;
  }

  static void FrameToBuffer(const Frame *frame, OIIO::ImageBuf* buf);

  static void BufferToFrame(OIIO::ImageBuf* buf, Frame* frame);

  static VideoParams::Format GetFormatFromOIIOBasetype(OIIO::TypeDesc::BASETYPE type);

  static rational GetPixelAspectRatioFromOIIO(const OIIO::ImageSpec& spec);

};

OLIVE_NAMESPACE_EXIT

#endif // OIIOUTILS_H
