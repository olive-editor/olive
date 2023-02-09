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

#include "common/ffmpegutils.h"

namespace olive {

AVPixelFormat FFmpegUtils::GetCompatiblePixelFormat(const AVPixelFormat &pix_fmt, PixelFormat maximum)
{
  AVPixelFormat possible_pix_fmts[3];

  possible_pix_fmts[0] = AV_PIX_FMT_RGBA;

  if (maximum == PixelFormat::U8) {
    possible_pix_fmts[1] = AV_PIX_FMT_NONE;
  } else {
    possible_pix_fmts[1] = AV_PIX_FMT_RGBA64;
    possible_pix_fmts[2] = AV_PIX_FMT_NONE;
  }

  return avcodec_find_best_pix_fmt_of_list(possible_pix_fmts,
                                           pix_fmt,
                                           1,
                                           nullptr);
}

SampleFormat FFmpegUtils::GetNativeSampleFormat(const AVSampleFormat &smp_fmt)
{
  switch (smp_fmt) {
  case AV_SAMPLE_FMT_U8:
    return SampleFormat::U8;
  case AV_SAMPLE_FMT_S16:
    return SampleFormat::S16;
  case AV_SAMPLE_FMT_S32:
    return SampleFormat::S32;
  case AV_SAMPLE_FMT_S64:
    return SampleFormat::S64;
  case AV_SAMPLE_FMT_FLT:
    return SampleFormat::F32;
  case AV_SAMPLE_FMT_DBL:
    return SampleFormat::F64;
  case AV_SAMPLE_FMT_U8P :
    return SampleFormat::U8P;
  case AV_SAMPLE_FMT_S16P:
    return SampleFormat::S16P;
  case AV_SAMPLE_FMT_S32P:
    return SampleFormat::S32P;
  case AV_SAMPLE_FMT_S64P:
    return SampleFormat::S64P;
  case AV_SAMPLE_FMT_FLTP:
    return SampleFormat::F32P;
  case AV_SAMPLE_FMT_DBLP:
    return SampleFormat::F64P;
  case AV_SAMPLE_FMT_NONE:
  case AV_SAMPLE_FMT_NB:
    break;
  }

  return SampleFormat::INVALID;
}

AVSampleFormat FFmpegUtils::GetFFmpegSampleFormat(const SampleFormat &smp_fmt)
{
  switch (smp_fmt) {
  case SampleFormat::U8:
    return AV_SAMPLE_FMT_U8;
  case SampleFormat::S16:
    return AV_SAMPLE_FMT_S16;
  case SampleFormat::S32:
    return AV_SAMPLE_FMT_S32;
  case SampleFormat::S64:
    return AV_SAMPLE_FMT_S64;
  case SampleFormat::F32:
    return AV_SAMPLE_FMT_FLT;
  case SampleFormat::F64:
    return AV_SAMPLE_FMT_DBL;
  case SampleFormat::U8P:
    return AV_SAMPLE_FMT_U8P;
  case SampleFormat::S16P:
    return AV_SAMPLE_FMT_S16P;
  case SampleFormat::S32P:
    return AV_SAMPLE_FMT_S32P;
  case SampleFormat::S64P:
    return AV_SAMPLE_FMT_S64P;
  case SampleFormat::F32P:
    return AV_SAMPLE_FMT_FLTP;
  case SampleFormat::F64P:
    return AV_SAMPLE_FMT_DBLP;
  case SampleFormat::INVALID:
  case SampleFormat::COUNT:
    break;
  }

  return AV_SAMPLE_FMT_NONE;
}

int FFmpegUtils::GetSwsColorspaceFromAVColorSpace(AVColorSpace cs)
{
  switch (cs) {
  case AVCOL_SPC_BT709:
    return SWS_CS_ITU709;
  case AVCOL_SPC_FCC:
    return SWS_CS_FCC;
  case AVCOL_SPC_BT470BG:
    return SWS_CS_ITU624;
  case AVCOL_SPC_SMPTE170M:
    return SWS_CS_SMPTE170M;
  case AVCOL_SPC_SMPTE240M:
    return SWS_CS_SMPTE240M;
  case AVCOL_SPC_BT2020_NCL:
    return SWS_CS_BT2020;
  default:
    break;
  }

  return SWS_CS_DEFAULT;
}

AVPixelFormat FFmpegUtils::ConvertJPEGSpaceToRegularSpace(AVPixelFormat f)
{
  switch (f) {
  case AV_PIX_FMT_YUVJ420P: return AV_PIX_FMT_YUV420P;
  case AV_PIX_FMT_YUVJ422P: return AV_PIX_FMT_YUV422P;
  case AV_PIX_FMT_YUVJ444P: return AV_PIX_FMT_YUV444P;
  case AV_PIX_FMT_YUVJ440P: return AV_PIX_FMT_YUV440P;
  case AV_PIX_FMT_YUVJ411P: return AV_PIX_FMT_YUV411P;
  default:
    break;
  }

  return f;
}

AVPixelFormat FFmpegUtils::GetFFmpegPixelFormat(const PixelFormat &pix_fmt, int channel_layout)
{
  if (channel_layout == VideoParams::kRGBChannelCount) {
    switch (pix_fmt) {
    case PixelFormat::U8:
      return AV_PIX_FMT_RGB24;
    case PixelFormat::U16:
      return AV_PIX_FMT_RGB48;
    case PixelFormat::F16:
    case PixelFormat::F32:
    case PixelFormat::INVALID:
    case PixelFormat::COUNT:
      break;
    }
  } else if (channel_layout == VideoParams::kRGBAChannelCount) {
    switch (pix_fmt) {
    case PixelFormat::U8:
      return AV_PIX_FMT_RGBA;
    case PixelFormat::U16:
      return AV_PIX_FMT_RGBA64;
    case PixelFormat::F16:
    case PixelFormat::F32:
    case PixelFormat::INVALID:
    case PixelFormat::COUNT:
      break;
    }
  }

  return AV_PIX_FMT_NONE;
}

PixelFormat FFmpegUtils::GetCompatiblePixelFormat(const PixelFormat &pix_fmt)
{
  switch (pix_fmt) {
  case PixelFormat::U8:
    return PixelFormat::U8;
  case PixelFormat::U16:
  case PixelFormat::F16:
  case PixelFormat::F32:
    return PixelFormat::U16;
  case PixelFormat::INVALID:
  case PixelFormat::COUNT:
    break;
  }

  return PixelFormat::INVALID;
}

}
