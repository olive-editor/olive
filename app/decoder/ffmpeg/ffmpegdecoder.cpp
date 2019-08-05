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

#include "render/pixelservice.h"

FFmpegDecoder::FFmpegDecoder() :
  fmt_ctx_(nullptr),
  codec_ctx_(nullptr),
  opts_(nullptr),
  scale_ctx_(nullptr),
  resample_ctx_(nullptr)
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

  // Set up
  if (codec_ctx_->codec_type == AVMEDIA_TYPE_VIDEO) {
    // Set up pixel format conversion for video
    AVPixelFormat pix_fmt = static_cast<AVPixelFormat>(avstream_->codecpar->format);

    // Get an Olive compatible AVPixelFormat
    AVPixelFormat ideal_pix_fmt = GetCompatiblePixelFormat(pix_fmt);

    // Determine which Olive native pixel format we retrieved
    // Note that FFmpeg doesn't support float formats
    switch (ideal_pix_fmt) {
    case AV_PIX_FMT_RGBA:
      output_fmt_ = olive::PIX_FMT_RGBA8;
      break;
    case AV_PIX_FMT_RGBA64:
      output_fmt_ = olive::PIX_FMT_RGBA16;
      break;
    default:
      // We should never get here, but if we do there's nothing we can do with this format
      return false;
    }

    scale_ctx_ = sws_getContext(avstream_->codecpar->width,
                                avstream_->codecpar->height,
                                pix_fmt,
                                avstream_->codecpar->width,
                                avstream_->codecpar->height,
                                ideal_pix_fmt,
                                0,
                                nullptr,
                                nullptr,
                                nullptr);

  } else if (codec_ctx_->codec_type == AVMEDIA_TYPE_AUDIO) {
    // FIXME: Fill this in
  }

  open_ = true;

  return true;
}

FramePtr FFmpegDecoder::Retrieve(const rational &timecode, const rational &length)
{
  if (!open_ && !Open()) {
    return nullptr;
  }

  //avcodec_flush_buffers(codec_ctx_);
  //av_seek_frame(fmt_ctx_, avstream_->index, 0, AVSEEK_FLAG_BACKWARD);

  // Allocate and init a packet for reading encoded data
  AVPacket pkt;
  av_init_packet(&pkt);

  // Allocate a new frame to place decoded data
  AVFrame* dec_frame = av_frame_alloc();

  // Cache FFmpeg error code returns
  int ret;

  // Variable set if FFmpeg signals file has finished
  bool eof = false;

  // FFmpeg frame retrieve loop
  while ((ret = avcodec_receive_frame(codec_ctx_, dec_frame)) == AVERROR(EAGAIN) && !eof) {

    // Find next packet in the correct stream index
    do {
      // Free buffer in packet if there is one
      if (pkt.buf != nullptr) {
        av_packet_unref(&pkt);
      }

      ret = av_read_frame(fmt_ctx_, &pkt);
    } while (pkt.stream_index != avstream_->index && ret >= 0);

    if (ret == AVERROR_EOF) {
      // Don't break so that receive gets called again, but don't try to read again
      eof = true;

      // Send a null packet to signal end of
      avcodec_send_packet(codec_ctx_, nullptr);
    } else if (ret < 0) {
      // Handle other error
      break;
    } else {
      // Successful read, send the packet
      ret = avcodec_send_packet(codec_ctx_, &pkt);

      // We don't need the packet anymore, so free it
      // FIXME: Is this true???
      av_packet_unref(&pkt);

      if (ret < 0) {
        break;
      }
    }
  }

//  qDebug() << dec_frame->pts << ",";

  // Handle any errors received during the frame retrieve process
  if (ret < 0) {
    qWarning() << tr("Failed to retrieve frame from FFmpeg decoder: %1").arg(ret);
    av_frame_free(&dec_frame);
    return nullptr;
  }

  // Frame was valid, now we create an Olive frame to place the data into
  FramePtr frame_container = std::make_shared<Frame>();
  frame_container->set_width(dec_frame->width);
  frame_container->set_height(dec_frame->height);
  frame_container->set_format(output_fmt_); // FIXME: Hardcoded value
  frame_container->set_timestamp(rational(dec_frame->pts * avstream_->time_base.num, avstream_->time_base.den));
  frame_container->allocate();

  // Convert pixel format/linesize if necessary
  uint8_t* dst_data = frame_container->data();
  int dst_linesize = frame_container->width() * 4;

  // Perform pixel conversion
  sws_scale(scale_ctx_,
            dec_frame->data,
            dec_frame->linesize,
            0,
            dec_frame->height,
            &dst_data,
            &dst_linesize);

  // Don't need AVFrame anymore
  av_frame_free(&dec_frame);

  Q_UNUSED(timecode)
  Q_UNUSED(length)

//   Close();

  return frame_container;
}

void FFmpegDecoder::Close()
{
  if (scale_ctx_ != nullptr) {
    sws_freeContext(scale_ctx_);
    scale_ctx_ = nullptr;
  }

  if (resample_ctx_ != nullptr) {
    swr_free(&resample_ctx_);
    resample_ctx_ = nullptr;
  }

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

AVPixelFormat FFmpegDecoder::GetCompatiblePixelFormat(const AVPixelFormat &pix_fmt)
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
