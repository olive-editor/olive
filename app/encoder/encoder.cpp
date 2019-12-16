#include "encoder.h"

Encoder::Encoder(const EncodingParams &params) :
  params_(params)
{
}

EncodingParams::EncodingParams() :
  video_enabled_(false),
  audio_enabled_(false)
{
}

void EncodingParams::SetFilename(const QString &filename)
{
  filename_ = filename;
}

void EncodingParams::EnableVideo(const VideoParams &video_params, const QString &vcodec)
{
  video_enabled_ = true;
  video_params_ = video_params;
  video_codec_ = vcodec;
}

void EncodingParams::EnableAudio(const AudioParams &audio_params, const QString &acodec)
{
  audio_enabled_ = true;
  audio_params_ = audio_params;
  audio_codec_ = acodec;
}
