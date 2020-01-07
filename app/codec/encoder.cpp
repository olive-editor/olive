#include "encoder.h"

#include "ffmpeg/ffmpegencoder.h"

Encoder::Encoder(const EncodingParams &params) :
  params_(params),
  open_(false)
{
}

const EncodingParams &Encoder::params() const
{
  return params_;
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

void EncodingParams::EnableVideo(const VideoRenderingParams &video_params, const QString &vcodec)
{
  video_enabled_ = true;
  video_params_ = video_params;
  video_codec_ = vcodec;
}

void EncodingParams::EnableAudio(const AudioRenderingParams &audio_params, const QString &acodec)
{
  audio_enabled_ = true;
  audio_params_ = audio_params;
  audio_codec_ = acodec;
}

const QString &EncodingParams::filename() const
{
  return filename_;
}

bool EncodingParams::video_enabled() const
{
  return video_enabled_;
}

const QString &EncodingParams::video_codec() const
{
  return video_codec_;
}

const VideoRenderingParams &EncodingParams::video_params() const
{
  return video_params_;
}

bool EncodingParams::audio_enabled() const
{
  return audio_enabled_;
}

const QString &EncodingParams::audio_codec() const
{
  return audio_codec_;
}

const AudioRenderingParams &EncodingParams::audio_params() const
{
  return audio_params_;
}

Encoder* Encoder::CreateFromID(const QString &id, const EncodingParams& params)
{
  return new FFmpegEncoder(params);
}

bool Encoder::IsOpen() const
{
  return open_;
}

void Encoder::Open()
{
  if (!open_) {
    open_ = OpenInternal();
  }

  if (open_) {
    emit OpenSucceeded();
  } else {
    emit OpenFailed();
  }
}

void Encoder::WriteFrame(FramePtr frame)
{
  if (open_) {
    WriteInternal(frame);
  }
}

void Encoder::Close()
{
  if (open_) {
    CloseInternal();

    open_ = false;
  }

  emit Closed();
}
