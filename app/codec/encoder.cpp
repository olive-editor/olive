/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "encoder.h"

#include "ffmpeg/ffmpegencoder.h"

OLIVE_NAMESPACE_ENTER

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
  video_bit_rate_(0),
  video_max_bit_rate_(0),
  video_buffer_size_(0),
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

void EncodingParams::SetVideoOption(const QString &key, const QString &value)
{
  video_opts_.insert(key, value);
}

void EncodingParams::SetVideoBitRate(const int64_t &rate)
{
  video_bit_rate_ = rate;
}

void EncodingParams::SetVideoMaxBitRate(const int64_t &rate)
{
  video_max_bit_rate_ = rate;
}

void EncodingParams::SetVideoBufferSize(const int64_t &sz)
{
  video_buffer_size_ = sz;
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

const QHash<QString, QString> &EncodingParams::video_opts() const
{
  return video_opts_;
}

const int64_t &EncodingParams::video_bit_rate() const
{
  return video_bit_rate_;
}

const int64_t &EncodingParams::video_max_bit_rate() const
{
  return video_max_bit_rate_;
}

const int64_t &EncodingParams::video_buffer_size() const
{
  return video_buffer_size_;
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

const rational &EncodingParams::GetExportLength() const
{
  return export_length_;
}

void EncodingParams::SetExportLength(const rational &export_length)
{
  export_length_ = export_length;
}

Encoder* Encoder::CreateFromID(const QString &id, const EncodingParams& params)
{
  Q_UNUSED(id)
  
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

void Encoder::WriteFrame(FramePtr frame, rational time)
{
  if (open_) {
    WriteInternal(frame, time);
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

OLIVE_NAMESPACE_EXIT
