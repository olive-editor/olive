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
  case kCodecH265:
    return tr("H.265");
  case kCodecOpenEXR:
    return tr("OpenEXR");
  case kCodecPNG:
    return tr("PNG");
  case kCodecProRes:
    return tr("ProRes");
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
  case kCodecH265:
  case kCodecProRes:
  case kCodecMP2:
  case kCodecMP3:
  case kCodecAAC:
  case kCodecPCM:
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

QStringList ExportCodec::GetPixelFormatsForCodec(ExportCodec::Codec c)
{
  QStringList pix_fmts;

  AVCodec* codec_info = nullptr;

  switch (c) {
  case kCodecH264:
    codec_info = avcodec_find_encoder(AV_CODEC_ID_H264);
    break;
  case kCodecDNxHD:
    codec_info = avcodec_find_encoder(AV_CODEC_ID_DNXHD);
    break;
  case kCodecProRes:
    codec_info = avcodec_find_encoder(AV_CODEC_ID_PRORES);
    break;
  case kCodecH265:
    codec_info = avcodec_find_encoder(AV_CODEC_ID_HEVC);
    break;
  case kCodecOpenEXR:
  case kCodecPNG:
  case kCodecTIFF:
    // FIXME: Add these in (these will most likely use an OIIOEncoder which doesn't exist yet)
    break;
  case kCodecMP2:
  case kCodecMP3:
  case kCodecAAC:
  case kCodecPCM:
  case kCodecCount:
    // These are audio or invalid codecs and therefore have no pixel formats
    break;
  }

  if (codec_info) {
    for (int i=0; codec_info->pix_fmts[i]!=-1; i++) {
      const char* pix_fmt_name = av_get_pix_fmt_name(codec_info->pix_fmts[i]);
      pix_fmts.append(pix_fmt_name);
    }
  }

  return pix_fmts;
}

}
