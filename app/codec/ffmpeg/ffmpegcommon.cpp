#include "ffmpegcommon.h"

AVPixelFormat FFmpegCommon::GetCompatiblePixelFormat(const AVPixelFormat &pix_fmt)
{
  AVPixelFormat possible_pix_fmts[] = {
    AV_PIX_FMT_RGBA,
    AV_PIX_FMT_RGBA64,
    AV_PIX_FMT_NONE
  };

  return avcodec_find_best_pix_fmt_of_list(possible_pix_fmts,
                                           pix_fmt,
                                           1,
                                           nullptr);
}

SampleFormat::Format FFmpegCommon::GetNativeSampleFormat(const AVSampleFormat &smp_fmt)
{
  switch (smp_fmt) {
  case AV_SAMPLE_FMT_U8:
    return SampleFormat::SAMPLE_FMT_U8;
  case AV_SAMPLE_FMT_S16:
    return SampleFormat::SAMPLE_FMT_S16;
  case AV_SAMPLE_FMT_S32:
    return SampleFormat::SAMPLE_FMT_S32;
  case AV_SAMPLE_FMT_S64:
    return SampleFormat::SAMPLE_FMT_S64;
  case AV_SAMPLE_FMT_FLT:
    return SampleFormat::SAMPLE_FMT_FLT;
  case AV_SAMPLE_FMT_DBL:
    return SampleFormat::SAMPLE_FMT_DBL;
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

  return SampleFormat::SAMPLE_FMT_INVALID;
}

AVSampleFormat FFmpegCommon::GetFFmpegSampleFormat(const SampleFormat::Format &smp_fmt)
{
  switch (smp_fmt) {
  case SampleFormat::SAMPLE_FMT_U8:
    return AV_SAMPLE_FMT_U8;
  case SampleFormat::SAMPLE_FMT_S16:
    return AV_SAMPLE_FMT_S16;
  case SampleFormat::SAMPLE_FMT_S32:
    return AV_SAMPLE_FMT_S32;
  case SampleFormat::SAMPLE_FMT_S64:
    return AV_SAMPLE_FMT_S64;
  case SampleFormat::SAMPLE_FMT_FLT:
    return AV_SAMPLE_FMT_FLT;
  case SampleFormat::SAMPLE_FMT_DBL:
    return AV_SAMPLE_FMT_DBL;
  case SampleFormat::SAMPLE_FMT_INVALID:
  case SampleFormat::SAMPLE_FMT_COUNT:
    break;
  }

  return AV_SAMPLE_FMT_NONE;
}

AVPixelFormat FFmpegCommon::GetFFmpegPixelFormat(const PixelFormat::Format &pix_fmt)
{
  switch (pix_fmt) {
  case PixelFormat::PIX_FMT_RGBA8:
    return AV_PIX_FMT_RGBA;
  case PixelFormat::PIX_FMT_RGBA16U:
    return AV_PIX_FMT_RGBA64;
  case PixelFormat::PIX_FMT_RGBA16F:
  case PixelFormat::PIX_FMT_RGBA32F:
  case PixelFormat::PIX_FMT_INVALID:
  case PixelFormat::PIX_FMT_COUNT:
    break;
  }

  return AV_PIX_FMT_NONE;
}

PixelFormat::Format FFmpegCommon::GetCompatiblePixelFormat(const PixelFormat::Format &pix_fmt)
{
  switch (pix_fmt) {
  case PixelFormat::PIX_FMT_RGBA8:
    return PixelFormat::PIX_FMT_RGBA8;
  case PixelFormat::PIX_FMT_RGBA16U:
  case PixelFormat::PIX_FMT_RGBA16F:
  case PixelFormat::PIX_FMT_RGBA32F:
    return PixelFormat::PIX_FMT_RGBA16U;
  case PixelFormat::PIX_FMT_INVALID:
  case PixelFormat::PIX_FMT_COUNT:
    break;
  }

  return PixelFormat::PIX_FMT_INVALID;
}
