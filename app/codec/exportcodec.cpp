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

#include "exportcodec.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
}

namespace olive {

QString ExportCodec::GetCodecName(ExportCodec::Codec c)
{
  switch (c) {
  case kCodecDNxHD:
    return tr("DNxHD");
  case kCodecH264:
    return tr("H.264");
  case kCodecH264rgb:
    return tr("H.264 RGB");
  case kCodecH265:
    return tr("H.265");
  case kCodecOpenEXR:
    return tr("OpenEXR");
  case kCodecPNG:
    return tr("PNG");
  case kCodecProRes:
    return tr("ProRes");
    case kCodecCineform:
    return tr("Cineform");
  case kCodecTIFF:
    return tr("TIFF");
  case kCodecMP2:
    return tr("MP2");
  case kCodecMP3:
    return tr("MP3");
  case kCodecAAC:
    return tr("AAC");
  case kCodecPCM:
    return tr("PCM (Uncompressed)");
  case kCodecFLAC:
    return tr("FLAC");
  case kCodecOpus:
    return tr("Opus");
  case kCodecVorbis:
    return tr("Vorbis");
  case kCodecVP9:
    return tr("VP9");
  case kCodecSRT:
    return tr("SubRip SRT");
  case kCodecCount:
    break;
  }

  return tr("Unknown");
}

bool ExportCodec::IsCodecAStillImage(ExportCodec::Codec c)
{
  switch (c) {
  case kCodecDNxHD:
  case kCodecH264:
  case kCodecH264rgb:
  case kCodecH265:
  case kCodecProRes:
  case kCodecCineform:
  case kCodecMP2:
  case kCodecMP3:
  case kCodecAAC:
  case kCodecPCM:
  case kCodecVorbis:
  case kCodecOpus:
  case kCodecFLAC:
  case kCodecVP9:
  case kCodecSRT:
    return false;
  case kCodecOpenEXR:
  case kCodecPNG:
  case kCodecTIFF:
    return true;
  case kCodecCount:
    break;
  }

  return false;
}

}
