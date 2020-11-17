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

#include "common/ffmpegutils.h"

OLIVE_NAMESPACE_ENTER

AVPixelFormat FFmpegUtils::GetCompatiblePixelFormat(const AVPixelFormat &pix_fmt)
{
  AVPixelFormat possible_pix_fmts[] = {
    AV_PIX_FMT_RGB24,
    AV_PIX_FMT_RGBA,
    AV_PIX_FMT_RGB48,
    AV_PIX_FMT_RGBA64,
    AV_PIX_FMT_NONE
  };

  return avcodec_find_best_pix_fmt_of_list(possible_pix_fmts,
                                           pix_fmt,
                                           1,
                                           nullptr);
}

AudioParams::Format FFmpegUtils::GetNativeSampleFormat(const AVSampleFormat &smp_fmt)
{
  switch (smp_fmt) {
  case AV_SAMPLE_FMT_U8:
    return AudioParams::kFormatUnsigned8;
  case AV_SAMPLE_FMT_S16:
    return AudioParams::kFormatSigned16;
  case AV_SAMPLE_FMT_S32:
    return AudioParams::kFormatSigned32;
  case AV_SAMPLE_FMT_S64:
    return AudioParams::kFormatSigned64;
  case AV_SAMPLE_FMT_FLT:
    return AudioParams::kFormatFloat32;
  case AV_SAMPLE_FMT_DBL:
    return AudioParams::kFormatFloat64;
  case AV_SAMPLE_FMT_U8P :
  case AV_SAMPLE_FMT_S16P:
  case AV_SAMPLE_FMT_S32P:
  case AV_SAMPLE_FMT_S64P:
  case AV_SAMPLE_FMT_FLTP:
  case AV_SAMPLE_FMT_DBLP:
  case AV_SAMPLE_FMT_NONE:
  case AV_SAMPLE_FMT_NB:
    break;
  }

  return AudioParams::kFormatInvalid;
}

AVSampleFormat FFmpegUtils::GetFFmpegSampleFormat(const AudioParams::Format &smp_fmt)
{
  switch (smp_fmt) {
  case AudioParams::kFormatUnsigned8:
    return AV_SAMPLE_FMT_U8;
  case AudioParams::kFormatSigned16:
    return AV_SAMPLE_FMT_S16;
  case AudioParams::kFormatSigned32:
    return AV_SAMPLE_FMT_S32;
  case AudioParams::kFormatSigned64:
    return AV_SAMPLE_FMT_S64;
  case AudioParams::kFormatFloat32:
    return AV_SAMPLE_FMT_FLT;
  case AudioParams::kFormatFloat64:
    return AV_SAMPLE_FMT_DBL;
  case AudioParams::kFormatInvalid:
  case AudioParams::kFormatCount:
    break;
  }

  return AV_SAMPLE_FMT_NONE;
}

AVPixelFormat FFmpegUtils::GetFFmpegPixelFormat(const VideoParams::Format &pix_fmt, int channel_layout)
{
  if (channel_layout == VideoParams::kRGBChannelCount) {
    switch (pix_fmt) {
    case VideoParams::kFormatUnsigned8:
      return AV_PIX_FMT_RGB24;
    case VideoParams::kFormatUnsigned16:
      return AV_PIX_FMT_RGB48;
    case VideoParams::kFormatFloat16:
    case VideoParams::kFormatFloat32:
    case VideoParams::kFormatInvalid:
    case VideoParams::kFormatCount:
      break;
    }
  } else if (channel_layout == VideoParams::kRGBAChannelCount) {
    switch (pix_fmt) {
    case VideoParams::kFormatUnsigned8:
      return AV_PIX_FMT_RGBA;
    case VideoParams::kFormatUnsigned16:
      return AV_PIX_FMT_RGBA64;
    case VideoParams::kFormatFloat16:
    case VideoParams::kFormatFloat32:
    case VideoParams::kFormatInvalid:
    case VideoParams::kFormatCount:
      break;
    }
  }

  return AV_PIX_FMT_NONE;
}

VideoParams::Format FFmpegUtils::GetCompatiblePixelFormat(const VideoParams::Format &pix_fmt)
{
  switch (pix_fmt) {
  case VideoParams::kFormatUnsigned8:
    return VideoParams::kFormatUnsigned8;
  case VideoParams::kFormatUnsigned16:
  case VideoParams::kFormatFloat16:
  case VideoParams::kFormatFloat32:
    return VideoParams::kFormatUnsigned16;
  case VideoParams::kFormatInvalid:
  case VideoParams::kFormatCount:
    break;
  }

  return VideoParams::kFormatInvalid;
}

OLIVE_NAMESPACE_EXIT
