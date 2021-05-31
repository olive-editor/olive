/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "ocioutils.h"

namespace olive {

OCIO::BitDepth OCIOUtils::GetOCIOBitDepthFromPixelFormat(VideoParams::Format format)
{
  switch (format) {
  case VideoParams::kFormatUnsigned8:
    return OCIO::BIT_DEPTH_UINT8;
  case VideoParams::kFormatUnsigned16:
    return OCIO::BIT_DEPTH_UINT16;
    break;
  case VideoParams::kFormatFloat16:
    return OCIO::BIT_DEPTH_F16;
    break;
  case VideoParams::kFormatFloat32:
    return OCIO::BIT_DEPTH_F32;
    break;
  case VideoParams::kFormatInvalid:
  case VideoParams::kFormatCount:
    break;
  }

  return OCIO::BIT_DEPTH_UNKNOWN;
}

}
