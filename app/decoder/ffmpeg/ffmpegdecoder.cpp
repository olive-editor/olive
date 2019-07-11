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

#include "ffmpegdecoder.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <QStatusBar>
#include <QString>
#include <QtMath>
#include <QDebug>

FFmpegDecoder::FFmpegDecoder() :
  fmt_ctx_(nullptr),
  codec_ctx_(nullptr),
  opts_(nullptr)
{
}

bool FFmpegDecoder::Open()
{
  if (open_) {
    return true;
  }

  int error_code;

  // Convert QString to a C strng
  QByteArray ba = stream()->footage()->filename().toUtf8();
  const char* filename = ba.constData();

  // Open file in a format context
  error_code = avformat_open_input(&fmt_ctx_, filename, nullptr, nullptr);

  // Handle format context error
  if (error_code != 0) {
    FFmpegErr(error_code);
    return false;
  }

  // Get stream information from format
  error_code = avformat_find_stream_info(fmt_ctx_, nullptr);

  // Handle get stream information error
  if (error_code < 0) {
    FFmpegErr(error_code);
    return false;
  }

  // Dump format information
  av_dump_format(fmt_ctx_, stream()->index(), filename, 0);

  // Get reference to correct AVStream
  avstream_ = fmt_ctx_->streams[stream()->index()];

  // Find decoder
  AVCodec* codec = avcodec_find_decoder(avstream_->codecpar->codec_id);

  // Handle failure to find decoder
  if (codec == nullptr) {
    Error(tr("Failed to find appropriate decoder for this codec (%1 :: %2)")
          .arg(stream()->footage()->filename(), avstream_->codecpar->codec_id));
    return false;
  }

  // Allocate context for the decoder
  codec_ctx_ = avcodec_alloc_context3(codec);
  if (codec_ctx_ == nullptr) {
    Error(tr("Failed to allocate codec context (%1 :: %2)").arg(stream()->footage()->filename(), stream()->index()));
    return false;
  }

  // Copy parameters from the AVStream to the AVCodecContext
  error_code = avcodec_parameters_to_context(codec_ctx_, avstream_->codecpar);

  // Handle failure to copy parameters
  if (error_code < 0) {
    FFmpegErr(error_code);
    return false;
  }

  // enable multithreading on decoding
  error_code = av_dict_set(&opts_, "threads", "auto", 0);

  // Handle failure to set multithreaded decoding
  if (error_code < 0) {
    FFmpegErr(error_code);
    return false;
  }

  // Open codec
  error_code = avcodec_open2(codec_ctx_, codec, &opts_);
  if (error_code < 0) {
    FFmpegErr(error_code);
    return false;
  }

  open_ = true;

  return true;
}

FramePtr FFmpegDecoder::Retrieve(const rational &timecode, const rational &length)
{
  // FIXME: Fill this out

  Q_UNUSED(timecode)
  Q_UNUSED(length)

  return nullptr;
}

void FFmpegDecoder::Close()
{
  if (opts_ != nullptr) {
    av_dict_free(&opts_);
    opts_ = nullptr;
  }

  if (codec_ctx_ != nullptr) {
    avcodec_free_context(&codec_ctx_);
    codec_ctx_ = nullptr;
  }

  if (fmt_ctx_ != nullptr) {
    avformat_close_input(&fmt_ctx_);
    fmt_ctx_ = nullptr;
  }

  open_ = false;
}

bool FFmpegDecoder::Probe(Footage *f)
{
  // Variable for receiving errors from FFmpeg
  int error_code;

  // Result to return
  bool result = false;

  // Convert QString to a C strng
  QByteArray ba = f->filename().toUtf8();
  const char* filename = ba.constData();

  // Open file in a format context
  error_code = avformat_open_input(&fmt_ctx_, filename, nullptr, nullptr);

  // Handle format context error
  if (error_code == 0) {

    // Retrieve metadata about the media
    av_dump_format(fmt_ctx_, 0, filename, 0);

    // Dump it into the Footage object
    for (unsigned int i=0;i<fmt_ctx_->nb_streams;i++) {

      avstream_ = fmt_ctx_->streams[i];

      Stream* str;

      if (avstream_->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {

        // Create a video stream object
        VideoStream* video_stream = new VideoStream();

        video_stream->set_width(avstream_->codecpar->width);
        video_stream->set_height(avstream_->codecpar->height);

        str = video_stream;

      } else if (avstream_->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {

        // Create an audio stream object
        AudioStream* audio_stream = new AudioStream();

        audio_stream->set_layout(avstream_->codecpar->channel_layout);
        audio_stream->set_channels(avstream_->codecpar->channels);
        audio_stream->set_sample_rate(avstream_->codecpar->sample_rate);

        str = audio_stream;

      } else {

        // This is data we can't utilize at the moment, but we make a Stream object anyway to keep parity with the file
        str = new Stream();

        // Set the correct codec type based on FFmpeg's result
        switch (avstream_->codecpar->codec_type) {
        case AVMEDIA_TYPE_UNKNOWN:
          str->set_type(Stream::kUnknown);
          break;
        case AVMEDIA_TYPE_DATA:
          str->set_type(Stream::kData);
          break;
        case AVMEDIA_TYPE_SUBTITLE:
          str->set_type(Stream::kSubtitle);
          break;
        case AVMEDIA_TYPE_ATTACHMENT:
          str->set_type(Stream::kAttachment);
          break;

        // We should never realistically get here, but we make an "invalid" stream just in case
        // We don't use a "default:" in case more AVMEDIA_TYPEs get introduced in later APIs that need handling
        case AVMEDIA_TYPE_NB:
        case AVMEDIA_TYPE_VIDEO:
        case AVMEDIA_TYPE_AUDIO:
          str->set_type(Stream::kUnknown);
          break;
        }

      }

      str->set_index(avstream_->index);
      str->set_timebase(avstream_->time_base);
      str->set_duration(avstream_->duration);

      f->add_stream(str);
    }

    // As long as we can open the container and retrieve information, this was a successful probe
    result = true;
  }

  // Free all memory
  Close();

  return result;
}

void FFmpegDecoder::FFmpegErr(int error_code)
{
  char err[1024];
  av_strerror(error_code, err, 1024);

  Error(tr("Error decoding %1 - %2 %3").arg(stream()->footage()->filename(),
                                            QString::number(error_code),
                                            err));
}

void FFmpegDecoder::Error(const QString &s)
{
  qWarning() << s;

  Close();
}
