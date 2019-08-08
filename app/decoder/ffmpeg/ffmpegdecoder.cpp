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

#include <QDebug>
#include <QFile>
#include <QString>
#include <QtMath>

#include "common/filefunctions.h"
#include "render/pixelservice.h"

FFmpegDecoder::FFmpegDecoder() :
  fmt_ctx_(nullptr),
  codec_ctx_(nullptr),
  opts_(nullptr),
  frame_(nullptr),
  pkt_(nullptr),
  scale_ctx_(nullptr),
  resample_ctx_(nullptr)
{
}

FFmpegDecoder::~FFmpegDecoder()
{
  Close();
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
    FFmpegError(error_code);
    return false;
  }

  // Get stream information from format
  error_code = avformat_find_stream_info(fmt_ctx_, nullptr);

  // Handle get stream information error
  if (error_code < 0) {
    FFmpegError(error_code);
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
    FFmpegError(error_code);
    return false;
  }

  // enable multithreading on decoding
  error_code = av_dict_set(&opts_, "threads", "auto", 0);

  // Handle failure to set multithreaded decoding
  if (error_code < 0) {
    FFmpegError(error_code);
    return false;
  }

  // Open codec
  error_code = avcodec_open2(codec_ctx_, codec, &opts_);
  if (error_code < 0) {
    FFmpegError(error_code);
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

  // Allocate a packet for reading
  pkt_ = av_packet_alloc();

  // Handle failure to create packet
  if (pkt_ == nullptr) {
    Error(tr("Failed to allocate AVPacket"));
    return false;
  }

  // Allocate a frame for decoding
  frame_ = av_frame_alloc();

  // Handle failure to create frame
  if (frame_ == nullptr) {
    Error(tr("Failed to allocate AVFrame"));
    return false;
  }

  // All allocation succeeded so we set the state to open
  open_ = true;

  return true;
}

FramePtr FFmpegDecoder::Retrieve(const rational &timecode, const rational &length)
{
  if (!open_ && !Open()) {
    return nullptr;
  }

  // Convert timecode to AVStream timebase
  int64_t target_ts = qFloor(timecode.toDouble() * rational(avstream_->time_base).flipped().toDouble());

  // Find closest actual timebase in the file
  target_ts = GetClosestTimestampInIndex(target_ts);

  if (target_ts < 0) {
    Error(tr("Index failed to produce a valid timestamp"));
    return nullptr;
  }

  // Cache FFmpeg error code returns
  int ret = 0;

  // Set up seeking loop
  int64_t seek_ts = target_ts;
  int64_t second_ts = qRound(rational(avstream_->time_base).flipped().toDouble());

  // FFmpeg frame retrieve loop
  while (ret >= 0 && frame_->pts != target_ts) {

    // If the frame timestamp is too large, we need to seek back a little
    if (frame_->pts > target_ts || frame_->pts == AV_NOPTS_VALUE) {
      avcodec_flush_buffers(codec_ctx_);
      av_seek_frame(fmt_ctx_, avstream_->index, seek_ts, AVSEEK_FLAG_BACKWARD);

      // FFmpeg doesn't always seek correctly, if we have to seek again we wrangle it into seeking back far enough

      // If we already tried seeking to 0 though, there's nothing we can do so we error here
      if (seek_ts == 0) {
        Error(tr("FFmpeg failed to seek to the correct location"));
        return nullptr;
      }

      seek_ts = qMax(0L, seek_ts - second_ts);
    }

    ret = GetFrame();
  }

  // Handle any errors received during the frame retrieve process
  if (ret < 0) {
    FFmpegError(ret);
    return nullptr;
  }

  // Frame was valid, now we create an Olive frame to place the data into
  FramePtr frame_container = std::make_shared<Frame>();
  frame_container->set_width(frame_->width);
  frame_container->set_height(frame_->height);
  frame_container->set_format(output_fmt_); // FIXME: Hardcoded value
  frame_container->set_timestamp(rational(frame_->pts * avstream_->time_base.num, avstream_->time_base.den));
  frame_container->allocate();

  // Convert pixel format/linesize if necessary
  uint8_t* dst_data = frame_container->data();
  int dst_linesize = frame_container->width() * PixelService::BytesPerPixel(static_cast<olive::PixelFormat>(output_fmt_));

  // Perform pixel conversion
  sws_scale(scale_ctx_,
            frame_->data,
            frame_->linesize,
            0,
            frame_->height,
            &dst_data,
            &dst_linesize);

  Q_UNUSED(length)

//   Close();

  return frame_container;
}

void FFmpegDecoder::Close()
{
  frame_index_.clear();

  if (scale_ctx_ != nullptr) {
    sws_freeContext(scale_ctx_);
    scale_ctx_ = nullptr;
  }

  if (resample_ctx_ != nullptr) {
    swr_free(&resample_ctx_);
    resample_ctx_ = nullptr;
  }

  if (pkt_ != nullptr) {
    av_packet_free(&pkt_);
    pkt_ = nullptr;
  }

  if (frame_ != nullptr) {
    av_frame_free(&frame_);
    frame_ = nullptr;
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

QString FFmpegDecoder::id()
{
  return "ffmpeg";
}

bool FFmpegDecoder::Probe(Footage *f)
{
  if (open_) {
    qWarning() << "Probe must be called while the Decoder is closed";
    return false;
  }

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

void FFmpegDecoder::FFmpegError(int error_code)
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

void FFmpegDecoder::Index()
{
  if (!open_) {
    qWarning() << tr("Indexing function tried to run while decoder was closed");
    return;
  }

  // This should be unnecessary, but just in case...
  frame_index_.clear();

  // Iterate through every single frame and get each timestamp
  // NOTE: Expects no frames to have been read so far

  int ret = 0;

  while (true) {
    ret = GetFrame();

    if (ret < 0) {
      break;
    } else {
      frame_index_.append(frame_->pts);
    }
  }

  // Save index to file
  SaveFrameIndex();
}

QString FFmpegDecoder::GetIndexFilename()
{
  if (!open_) {
    qWarning() << tr("GetIndexFilename tried to run while decoder was closed");
    return QString();
  }

  return GetMediaIndexFilename(GetUniqueFileIdentifier(stream()->footage()->filename()))
      .append(QString::number(avstream_->index));
}

bool FFmpegDecoder::LoadFrameIndex()
{
  // Load index from file
  QFile index_file(GetIndexFilename());

  if (!index_file.exists()) {
    return false;
  }

  if (index_file.open(QFile::ReadOnly)) {
    // Resize based on filesize
    frame_index_.resize(static_cast<int>(static_cast<size_t>(index_file.size()) / sizeof(int64_t)));

    // Read frame index into vector
    index_file.read(reinterpret_cast<char*>(frame_index_.data()),
                    index_file.size());

    index_file.close();

    return true;
  }

  return false;
}

void FFmpegDecoder::SaveFrameIndex()
{
  // Save index to file
  QFile index_file(GetIndexFilename());
  if (index_file.open(QFile::WriteOnly)) {
    // Write index in binary
    index_file.write(reinterpret_cast<const char*>(frame_index_.constData()),
                     frame_index_.size() * static_cast<int>(sizeof(int64_t)));

    index_file.close();
  } else {
    qWarning() << tr("Failed to save index for %1").arg(stream()->footage()->filename());
  }
}

int FFmpegDecoder::GetFrame()
{
  bool eof = false;

  int ret;

  // Clear any previous frames
  av_frame_unref(frame_);

  while ((ret = avcodec_receive_frame(codec_ctx_, frame_)) == AVERROR(EAGAIN) && !eof) {

    // Find next packet in the correct stream index
    do {
      // Free buffer in packet if there is one
      av_packet_unref(pkt_);

      // Read packet from file
      ret = av_read_frame(fmt_ctx_, pkt_);
    } while (pkt_->stream_index != avstream_->index && ret >= 0);

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
      ret = avcodec_send_packet(codec_ctx_, pkt_);

      // We don't need the packet anymore, so free it
      av_packet_unref(pkt_);

      if (ret < 0) {
        break;
      }
    }
  }

  return ret;
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

int64_t FFmpegDecoder::GetClosestTimestampInIndex(const int64_t &ts)
{
  // Index now if we haven't already
  if (frame_index_.isEmpty() && !LoadFrameIndex()) {
    Index();
  }

  // Use index to find closest frame in file
  for (int i=1;i<frame_index_.size();i++) {
    if (frame_index_.at(i) == ts) {
      return ts;
    } else if (frame_index_.at(i) > ts) {
      return frame_index_.at(i - 1);
    }
  }

  return -1;
}
