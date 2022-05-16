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

#include "ffmpegdecoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
}

#include <OpenImageIO/imagebuf.h>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QtMath>
#include <QThread>
#include <QtConcurrent/QtConcurrent>

#include "codec/planarfiledevice.h"
#include "common/define.h"
#include "common/ffmpegutils.h"
#include "common/filefunctions.h"
#include "common/functiontimer.h"
#include "common/timecodefunctions.h"
#include "render/framehashcache.h"
#include "render/diskmanager.h"
#include "render/subtitleparams.h"

namespace olive {

FFmpegDecoder::FFmpegDecoder() :
  filter_graph_(nullptr),
  buffersrc_ctx_(nullptr),
  buffersink_ctx_(nullptr),
  working_frame_(nullptr),
  working_packet_(nullptr),
  is_working_(false),
  cache_at_zero_(false),
  cache_at_eof_(false)
{
}

bool FFmpegDecoder::OpenInternal()
{
  if (instance_.Open(stream().filename().toUtf8(), stream().stream())) {
    AVStream* s = instance_.avstream();

    // Store one second in the source's timebase
    second_ts_ = qRound64(av_q2d(av_inv_q(s->time_base)));

    if (s->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      // Get an Olive compatible AVPixelFormat
      ideal_pix_fmt_ = FFmpegUtils::GetCompatiblePixelFormat(static_cast<AVPixelFormat>(s->codecpar->format));

      // Determine which Olive native pixel format we retrieved
      // Note that FFmpeg doesn't support float formats
      native_pix_fmt_ = GetNativePixelFormat(ideal_pix_fmt_);
      native_channel_count_ = GetNativeChannelCount(ideal_pix_fmt_);

      if (native_pix_fmt_ == VideoParams::kFormatInvalid
          || native_channel_count_ == 0) {
        qCritical() << "Failed to find valid native pixel format for" << ideal_pix_fmt_;
        return false;
      }
    }

    working_frame_ = av_frame_alloc();
    working_packet_ = av_packet_alloc();

    return true;
  }

  return false;
}

/*FramePtr FFmpegDecoder::RetrieveStillImage(const rational &timecode, const int &divider)
{
  // This is a still image
  QString img_filename = stream().filename();

  int64_t ts;

  // If it's an image sequence, we'll probably need to transform the filename
  if (stream().GetStream().video_type() == Track::kVideoTypeImageSequence) {
    ts = stream().GetTimeInTimebaseUnits(timecode);

    img_filename = TransformImageSequenceFileName(stream().filename(), ts);
  } else {
    ts = 0;
  }

  AVPacket* pkt = av_packet_alloc();
  AVFrame* frame = av_frame_alloc();
  FramePtr output_frame = nullptr;

  Instance i;
  i.Open(img_filename.toUtf8(), stream().GetRealStreamIndex());

  int ret = i.GetFrame(pkt, frame);

  if (ret >= 0) {
    VideoParams video_params = stream().video_params();

    // Create frame to return
    output_frame = Frame::Create();
    output_frame->set_video_params(VideoParams(frame->width,
                                               frame->height,
                                               native_pix_fmt_,
                                               native_channel_count_,
                                               video_params.pixel_aspect_ratio(),
                                               video_params.interlacing(),
                                               divider));
    output_frame->set_timestamp(timecode);
    output_frame->allocate();

    uint8_t* copy_data = reinterpret_cast<uint8_t*>(output_frame->data());
    int copy_linesize = output_frame->linesize_bytes();

    FFmpegBufferToNativeBuffer(frame->data, frame->linesize, &copy_data, &copy_linesize);
  } else {
    qWarning() << "Failed to retrieve still image from decoder";
  }

  i.Close();

  av_frame_free(&frame);
  av_packet_free(&pkt);

  return output_frame;
}*/

FramePtr FFmpegDecoder::RetrieveVideoInternal(const rational &timecode, const RetrieveVideoParams &params, const QAtomicInt *cancelled)
{
  if (!InitScaler(params)) {
    return nullptr;
  }

  return RetrieveFrame(timecode, cancelled);
}

void FFmpegDecoder::CloseInternal()
{
  if (working_packet_) {
    av_packet_free(&working_packet_);
    working_packet_ = nullptr;
  }

  if (working_frame_) {
    av_frame_free(&working_frame_);
    working_frame_ = nullptr;
  }

  ClearFrameCache();

  instance_.Close();
}

int FFmpegDecoder::GetFilteredFrame(AVPacket* packet, AVFrame* output_frame)
{
  // Ensure scaler is correct for these parameters
  int ret;

  // Try to pull frame from buffersink
  while ((ret = av_buffersink_get_frame(buffersink_ctx_, output_frame)) == AVERROR(EAGAIN)) {
    // If no frame is ready in the buffersink, pull from codec
    ret = instance_.GetFrame(packet, output_frame);

    if (ret >= 0) {
      // Override this frame's interlacing parameters from user
      switch (filter_params_.src_interlacing) {
      case VideoParams::kInterlaceNone:
        output_frame->interlaced_frame = 0;
        break;
      case VideoParams::kInterlacedTopFirst:
        output_frame->interlaced_frame = 1;
        output_frame->top_field_first = 1;
        break;
      case VideoParams::kInterlacedBottomFirst:
        output_frame->interlaced_frame = 1;
        output_frame->top_field_first = 0;
        break;
      }

      // If succeeded in pulling from codec, send to buffer source
      ret = av_buffersrc_add_frame_flags(buffersrc_ctx_, output_frame, AV_BUFFERSRC_FLAG_KEEP_REF);

      av_frame_unref(output_frame);

      if (ret < 0) {
        // If failed to send to buffer source, return break and error code
        qCritical() << "Failed to feed filter graph:" << FFmpegError(ret);
        break;
      }
    } else {
      // If failed to read from decoder, return break and error code
      break;
    }
  }

  return ret;
}

QString FFmpegDecoder::id() const
{
  return QStringLiteral("ffmpeg");
}

FootageDescription FFmpegDecoder::Probe(const QString &filename, const QAtomicInt *cancelled) const
{
  // Return value
  FootageDescription desc(id());

  // Variable for receiving errors from FFmpeg
  int error_code;

  // Convert QString to a C string
  QByteArray filename_c = filename.toUtf8();

  // Open file in a format context
  AVFormatContext* fmt_ctx = nullptr;
  error_code = avformat_open_input(&fmt_ctx, filename_c, nullptr, nullptr);

  // Handle format context error
  if (error_code == 0) {

    // Retrieve metadata about the media
    avformat_find_stream_info(fmt_ctx, nullptr);

    int64_t footage_duration = fmt_ctx->duration;

    // Dump it into the Footage object
    for (unsigned int i=0;i<fmt_ctx->nb_streams;i++) {

      // FFmpeg AVStream
      AVStream* avstream = fmt_ctx->streams[i];

      // Find decoder for this stream, if it exists we can proceed
      const AVCodec* decoder = avcodec_find_decoder(avstream->codecpar->codec_id);

      if (decoder
          && (avstream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
              || avstream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO
              || avstream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)) {

        if (avstream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {

          bool image_is_still = false;
          rational pixel_aspect_ratio;
          rational frame_rate;
          VideoParams::Interlacing interlacing = VideoParams::kInterlaceNone;

          {
            // Read at least two frames to get more information about this video stream
            AVPacket* pkt = av_packet_alloc();
            AVFrame* frame = av_frame_alloc();

            {
              Instance instance;
              instance.Open(filename_c, avstream->index);

              // Read first frame and retrieve some metadata
              if (instance.GetFrame(pkt, frame) >= 0) {
                // Check if video is interlaced and what field dominance it has if so
                if (frame->interlaced_frame) {
                  if (frame->top_field_first) {
                    interlacing = VideoParams::kInterlacedTopFirst;
                  } else {
                    interlacing = VideoParams::kInterlacedBottomFirst;
                  }
                }

                pixel_aspect_ratio = av_guess_sample_aspect_ratio(instance.fmt_ctx(),
                                                                  instance.avstream(),
                                                                  frame);

                frame_rate = av_guess_frame_rate(instance.fmt_ctx(),
                                                 instance.avstream(),
                                                 frame);
              }

              // Read second frame
              int ret = instance.GetFrame(pkt, frame);

              if (ret >= 0) {
                // Check if we need a manual duration
                if (avstream->duration == AV_NOPTS_VALUE) {
                  if (footage_duration == AV_NOPTS_VALUE) {

                    // Manually read through file for duration
                    int64_t new_dur;

                    do {
                      new_dur = frame->best_effort_timestamp;
                    } while (instance.GetFrame(pkt, frame) >= 0);

                    avstream->duration = new_dur;

                  } else {

                    // Fallback to footage duration
                    avstream->duration = Timecode::rescale_timestamp_ceil(footage_duration, rational(1, AV_TIME_BASE), avstream->time_base);

                  }
                }
              } else if (ret == AVERROR_EOF) {
                // Video has only one frame in it, treat it like a still image
                image_is_still = true;
              }

              instance.Close();
            }

            av_frame_free(&frame);
            av_packet_free(&pkt);
          }

          AVPixelFormat compatible_pix_fmt = FFmpegUtils::GetCompatiblePixelFormat(static_cast<AVPixelFormat>(avstream->codecpar->format));

          VideoParams stream;
          stream.set_stream_index(i);
          stream.set_width(avstream->codecpar->width);
          stream.set_height(avstream->codecpar->height);
          stream.set_video_type((image_is_still) ? VideoParams::kVideoTypeStill : VideoParams::kVideoTypeVideo);
          stream.set_format(GetNativePixelFormat(compatible_pix_fmt));
          stream.set_channel_count(GetNativeChannelCount(compatible_pix_fmt));
          stream.set_interlacing(interlacing);
          stream.set_pixel_aspect_ratio(pixel_aspect_ratio);
          stream.set_frame_rate(frame_rate);
          stream.set_start_time(avstream->start_time);
          stream.set_time_base(avstream->time_base);
          stream.set_duration(avstream->duration);

          // Defaults to false, requires user intervention if incorrect
          stream.set_premultiplied_alpha(false);

          desc.AddVideoStream(stream);

        } else if (avstream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {

          // Create an audio stream object
          uint64_t channel_layout = avstream->codecpar->channel_layout;
          if (!channel_layout) {
            channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(avstream->codecpar->channels));
          }

          if (avstream->duration == AV_NOPTS_VALUE) {
            // Loop through stream until we get the whole duration
            if (footage_duration == AV_NOPTS_VALUE) {
              Instance instance;
              instance.Open(filename_c, avstream->index);

              AVPacket* pkt = av_packet_alloc();
              AVFrame* frame = av_frame_alloc();

              int64_t new_dur;

              do {
                new_dur = frame->best_effort_timestamp;
              } while (instance.GetFrame(pkt, frame) >= 0);

              avstream->duration = new_dur;

              av_frame_free(&frame);
              av_packet_free(&pkt);

              instance.Close();
            } else {

              avstream->duration = Timecode::rescale_timestamp_ceil(footage_duration, rational(1, AV_TIME_BASE), avstream->time_base);

            }
          }

          AudioParams stream;
          stream.set_stream_index(i);
          stream.set_channel_layout(channel_layout);
          stream.set_sample_rate(avstream->codecpar->sample_rate);
          stream.set_format(AudioParams::kInternalFormat);
          stream.set_time_base(avstream->time_base);
          stream.set_duration(avstream->duration);
          desc.AddAudioStream(stream);

        } else if (avstream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) {

          // Limit to SRT for now...
          if (avstream->codecpar->codec_id == AV_CODEC_ID_SUBRIP) {
            SubtitleParams sub;

            AVPacket* pkt = av_packet_alloc();
            {
              Instance instance;
              instance.Open(filename_c, avstream->index);

              while (instance.GetPacket(pkt) >= 0) {
                TimeRange time(Timecode::timestamp_to_time(pkt->pts, avstream->time_base),
                               Timecode::timestamp_to_time(pkt->pts + pkt->duration, avstream->time_base));

                QString text = QString::fromUtf8((const char *) pkt->data, pkt->size);

                sub.push_back(Subtitle(time, text));
              }

              instance.Close();
            }
            av_packet_free(&pkt);

            desc.AddSubtitleStream(sub);
          }

        }

      }

    }

  }

  // Free all memory
  avformat_close_input(&fmt_ctx);

  return desc;
}

QString FFmpegDecoder::FFmpegError(int error_code)
{
  char err[1024];
  av_strerror(error_code, err, 512);
  return QStringLiteral("%1 %2").arg(QString::number(error_code), err);
}

bool FFmpegDecoder::ConformAudioInternal(const QVector<QString> &filenames, const AudioParams &params, const QAtomicInt *cancelled)
{
  // Iterate through each audio frame and extract the PCM data

  // Seek to starting point
  instance_.Seek(0);

  // Handle NULL channel layout
  uint64_t channel_layout = ValidateChannelLayout(instance_.avstream());
  if (!channel_layout) {
    qCritical() << "Failed to determine channel layout of audio file, could not conform";
    return false;
  }

  // Create resampling context
  SwrContext* resampler = swr_alloc_set_opts(nullptr,
                                             params.channel_layout(),
                                             FFmpegUtils::GetFFmpegSampleFormat(params.format()),
                                             params.sample_rate(),
                                             channel_layout,
                                             static_cast<AVSampleFormat>(instance_.avstream()->codecpar->format),
                                             instance_.avstream()->codecpar->sample_rate,
                                             0,
                                             nullptr);

  swr_init(resampler);

  AVPacket* pkt = av_packet_alloc();
  AVFrame* frame = av_frame_alloc();
  int ret;

  bool success = false;

  int64_t duration = instance_.avstream()->duration;
  if (duration == 0 || duration == AV_NOPTS_VALUE) {
    duration = instance_.fmt_ctx()->duration;
    if (!(duration == 0 || duration == AV_NOPTS_VALUE)) {
      // Rescale from AVFormatContext timebase to AVStream timebase
      duration = av_rescale_q_rnd(duration, {1, AV_TIME_BASE}, instance_.avstream()->time_base, AV_ROUND_UP);
    }
  }

  PlanarFileDevice wave_out;
  if (wave_out.open(filenames, QFile::WriteOnly)) {
    int nb_channels = params.channel_count();
    SampleBuffer data;
    data.set_audio_params(params);

    while (true) {
      // Check if we have a `cancelled` ptr and its value
      if (cancelled && *cancelled) {
        break;
      }

      ret = instance_.GetFrame(pkt, frame);

      if (ret < 0) {

        if (ret == AVERROR_EOF) {
          success = true;
        } else {
          char err_str[512];
          av_strerror(ret, err_str, 512);
          qWarning() << "Failed to conform:" << ret << err_str;
        }
        break;

      }

      // Allocate buffers
      int nb_samples = swr_get_out_samples(resampler, frame->nb_samples);
      int nb_bytes_per_channel = params.samples_to_bytes(nb_samples) / nb_channels;
      data.set_sample_count(nb_bytes_per_channel);
      data.allocate();

      // Resample audio to our destination parameters
      nb_samples = swr_convert(resampler,
                               reinterpret_cast<uint8_t**>(data.to_raw_ptrs().data()),
                               nb_samples,
                               const_cast<const uint8_t**>(frame->data),
                               frame->nb_samples);

      // If no error, write to files
      if (nb_samples > 0) {
        // Update byte count for the number of samples we actually received
        nb_bytes_per_channel = params.samples_to_bytes(nb_samples) / nb_channels;

        // Write to files
        wave_out.write(const_cast<const char**>(reinterpret_cast<char**>(data.to_raw_ptrs().data())), nb_bytes_per_channel);
      }

      // Free buffer
      data.destroy();

      // Handle error now after freeing
      if (nb_samples < 0) {
        char err_str[512];
        av_strerror(nb_samples, err_str, 512);
        qWarning() << "libswresample failed with error:" << nb_samples << err_str;
        break;
      }

      SignalProcessingProgress(frame->best_effort_timestamp, duration);
    }

    wave_out.close();
  } else {
    qWarning() << "Failed to open WAVE output for indexing";
  }

  swr_free(&resampler);

  av_frame_free(&frame);
  av_packet_free(&pkt);

  return success;
}

VideoParams::Format FFmpegDecoder::GetNativePixelFormat(AVPixelFormat pix_fmt)
{
  switch (pix_fmt) {
  case AV_PIX_FMT_RGB24:
  case AV_PIX_FMT_RGBA:
    return VideoParams::kFormatUnsigned8;
  case AV_PIX_FMT_RGB48:
  case AV_PIX_FMT_RGBA64:
    return VideoParams::kFormatUnsigned16;
  default:
    return VideoParams::kFormatInvalid;
  }
}

int FFmpegDecoder::GetNativeChannelCount(AVPixelFormat pix_fmt)
{
  switch (pix_fmt) {
  case AV_PIX_FMT_RGB24:
  case AV_PIX_FMT_RGB48:
    return VideoParams::kRGBChannelCount;
  case AV_PIX_FMT_RGBA:
  case AV_PIX_FMT_RGBA64:
    return VideoParams::kRGBAChannelCount;
  default:
    return 0;
  }
}

uint64_t FFmpegDecoder::ValidateChannelLayout(AVStream* stream)
{
  if (stream->codecpar->channel_layout) {
    return stream->codecpar->channel_layout;
  }

  return av_get_default_channel_layout(stream->codecpar->channels);
}

const char *FFmpegDecoder::GetInterlacingModeInFFmpeg(VideoParams::Interlacing interlacing)
{
  if (interlacing == VideoParams::kInterlacedTopFirst) {
    return "tff";
  } else {
    return "bff";
  }
}

/* OLD UNUSED CODE: Keeping this around in case the code proves useful

void FFmpegDecoder::CacheFrameToDisk(AVFrame *f)
{
  QFile save_frame(GetIndexFilename().append(QString::number(f->pts)));
  if (save_frame.open(QFile::WriteOnly)) {

    // Save frame to media index
    int cached_buffer_sz = av_image_get_buffer_size(static_cast<AVPixelFormat>(f->format),
                                                    f->width,
                                                    f->height,
                                                    1);

    QByteArray cached_frame(cached_buffer_sz, Qt::Uninitialized);

    av_image_copy_to_buffer(reinterpret_cast<uint8_t*>(cached_frame.data()),
                            cached_frame.size(),
                            f->data,
                            f->linesize,
                            static_cast<AVPixelFormat>(f->format),
                            f->width,
                            f->height,
                            1);

    save_frame.write(qCompress(cached_frame, 1));
    save_frame.close();

    DiskManager::instance()->CreatedFile(save_frame.fileName(), QByteArray());
  }

  // See if we stored this frame in the disk cache

  QByteArray frame_loader;
  if (!got_frame) {
    QFile compressed_frame(GetIndexFilename().append(QString::number(target_ts)));
    if (compressed_frame.exists()
        && compressed_frame.size() > 0
        && compressed_frame.open(QFile::ReadOnly)) {
      DiskManager::instance()->Accessed(compressed_frame.fileName());

      // Read data
      frame_loader = qUncompress(compressed_frame.readAll());

      av_image_fill_arrays(input_data,
                           input_linesize,
                           reinterpret_cast<uint8_t*>(frame_loader.data()),
                           static_cast<AVPixelFormat>(avstream_->codecpar->format),
                           avstream_->codecpar->width,
                           avstream_->codecpar->height,
                           1);

      got_frame = true;
    }
  }
}
*/

void FFmpegDecoder::ClearFrameCache()
{
  if (!cached_frames_.empty()) {
    cached_frames_.clear();
    cache_at_eof_ = false;
    cache_at_zero_ = false;

    // Filter graph may rely on "continuous" video frames, so we free the scaler here
    FreeScaler();
    InitScaler(filter_params_);
  }
}

FramePtr FFmpegDecoder::RetrieveFrame(const rational& time, const QAtomicInt *cancelled)
{
  int64_t target_ts = GetTimeInTimebaseUnits(time, instance_.avstream()->time_base, instance_.avstream()->start_time, filter_params_.src_interlacing);

  const int64_t min_seek = -instance_.avstream()->start_time;
  int64_t seek_ts = target_ts;
  bool still_seeking = false;

  if (time != kAnyTimecode) {
    // If the frame wasn't in the frame cache, see if this frame cache is too old to use
    if (cached_frames_.empty()
        || (time < cached_frames_.front()->timestamp() || time > cached_frames_.back()->timestamp() + 2)) {
      ClearFrameCache();

      instance_.Seek(seek_ts);
      if (seek_ts == min_seek) {
        cache_at_zero_ = true;
      }

      still_seeking = true;
    } else {
      // Search cache for frame
      FramePtr cached_frame = GetFrameFromCache(time);
      if (cached_frame) {
        return cached_frame;
      }
    }
  }

  int ret;
  FramePtr return_frame = nullptr;

  while (true) {
    // Break out of loop if we've cancelled
    if (cancelled && *cancelled) {
      break;
    }

    // Pull from the decoder
    av_frame_unref(working_frame_);
    ret = GetFilteredFrame(working_packet_, working_frame_);

    // Handle any errors that aren't EOF (EOF is handled later on)
    if (ret < 0 && ret != AVERROR_EOF) {
      qCritical() << "Failed to retrieve frame:" << ret;
      break;
    }

    if (still_seeking) {
      // Handle a failure to seek (occurs on some media)
      // We'll only be here if the frame cache was emptied earlier
      if (!cache_at_zero_ && (ret == AVERROR_EOF || working_frame_->best_effort_timestamp > target_ts)) {

        seek_ts = qMax(min_seek, seek_ts - second_ts_);
        instance_.Seek(seek_ts);
        if (seek_ts == min_seek) {
          cache_at_zero_ = true;
        }
        continue;

      } else {

        still_seeking = false;

      }
    }


    if (ret == AVERROR_EOF) {

      // Handle an "expected" EOF by using the last frame of our cache
      cache_at_eof_ = true;

      if (cached_frames_.empty()) {
        qCritical() << "Unexpected codec EOF - unable to retrieve frame";
      } else {
        return_frame = cached_frames_.back();
      }

      break;

    } else {

      // Cut down to thread count - 1 before we acquire a new frame
      if (cached_frames_.size() == size_t(QThread::idealThreadCount())) {
        RemoveFirstFrame();
      }

      FramePtr cached = Frame::Create();
      cached->set_video_params(GetVideoParams());
      cached->allocate();

      // Store in queue, converting to native format
      uint8_t* destination_data = reinterpret_cast<uint8_t*>(cached->data());
      int destination_linesize = cached->linesize_bytes();

      av_image_copy(&destination_data, &destination_linesize, const_cast<const uint8_t**>(working_frame_->data), working_frame_->linesize, static_cast<AVPixelFormat>(working_frame_->format), working_frame_->width, working_frame_->height);

      // Set timestamp so this frame can be identified later
      cached->set_timestamp(GetTimestampInTimeUnits(working_frame_->best_effort_timestamp, instance_.avstream()->time_base, instance_.avstream()->start_time, filter_params_.src_interlacing));

      // Store frame before just in case
      FramePtr previous;
      if (cached_frames_.empty()) {
        previous = nullptr;
      } else {
        previous = cached_frames_.back();
      }

      // Append this frame and signal to other threads that a new frame has arrived
      cached_frames_.push_back(cached);

      // If this is a valid frame, see if this or the frame before it are the one we need
      if (cached->timestamp() == time || time == kAnyTimecode) {
        return_frame = cached;
        break;
      } else if (cached->timestamp() > time) {
        if (!previous && cache_at_zero_) {
          return_frame = cached;
          break;
        } else {
          return_frame = previous;
          break;
        }
      }
    }
  }

  av_frame_unref(working_frame_);
  av_packet_unref(working_packet_);

  return return_frame;
}

bool FFmpegDecoder::InitScaler(const RetrieveVideoParams& params)
{
  if (params == filter_params_ && filter_graph_) {
    // We have an appropriate filter for these parameters, just return true
    return true;
  }

  // We need to (re)create the filter, delete current if necessary
  ClearFrameCache();

  // Set our params to this
  filter_params_ = params;

  // Allocate filter graph
  filter_graph_ = avfilter_graph_alloc();
  if (!filter_graph_) {
    qWarning() << "Failed to allocate filter graph";
    return false;
  }

  AVStream* s = instance_.avstream();

  int src_width = s->codecpar->width;
  int src_height = s->codecpar->height;

  // Define filter parameters
  static const int kFilterArgSz = 1024;
  char filter_args[kFilterArgSz];
  snprintf(filter_args, kFilterArgSz, "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
           src_width,
           src_height,
           s->codecpar->format,
           s->time_base.num,
           s->time_base.den,
           s->codecpar->sample_aspect_ratio.num,
           s->codecpar->sample_aspect_ratio.den);

  // Create path in and out of the filter graph (the buffer in and the buffersink out)
  avfilter_graph_create_filter(&buffersrc_ctx_, avfilter_get_by_name("buffer"), "in", filter_args, nullptr, filter_graph_);
  avfilter_graph_create_filter(&buffersink_ctx_, avfilter_get_by_name("buffersink"), "out", nullptr, nullptr, filter_graph_);

  // Link filters as necessary
  AVFilterContext *last_filter = buffersrc_ctx_;

  // Add interlacing filter if necessary
  if (filter_params_.src_interlacing != VideoParams::kInterlaceNone) {
    // Footage is interlaced, our renderer works in progressive so we'll need to de-interlace
    AVFilterContext* interlace_filter;

    snprintf(filter_args, kFilterArgSz, "mode=1:parity=%s",
             GetInterlacingModeInFFmpeg(filter_params_.src_interlacing));
    avfilter_graph_create_filter(&interlace_filter, avfilter_get_by_name("yadif"), "yadif", filter_args, nullptr, filter_graph_);

    avfilter_link(last_filter, 0, interlace_filter, 0);
    last_filter = interlace_filter;
  }

  // Add scale filter if necessary
  int dst_width, dst_height;
  if (filter_params_.divider > 1) {
    AVFilterContext* scale_filter;

    dst_width = VideoParams::GetScaledDimension(src_width, filter_params_.divider);
    dst_height = VideoParams::GetScaledDimension(src_height, filter_params_.divider);

    snprintf(filter_args, kFilterArgSz, "w=%d:h=%d:flags=fast_bilinear:interl=%d",
             dst_width,
             dst_height,
             params.dst_interlacing != VideoParams::kInterlaceNone);

    avfilter_graph_create_filter(&scale_filter, avfilter_get_by_name("scale"), "scale", filter_args, nullptr, filter_graph_);

    avfilter_link(last_filter, 0, scale_filter, 0);
    last_filter = scale_filter;
  } else {
    dst_width = src_width;
    dst_height = src_height;
  }

  // Add format filter if necessary
  if (ideal_pix_fmt_ != s->codecpar->format) {
    AVFilterContext* format_filter;

    snprintf(filter_args, kFilterArgSz, "pix_fmts=%u", ideal_pix_fmt_);

    avfilter_graph_create_filter(&format_filter, avfilter_get_by_name("format"), "format", filter_args, nullptr, filter_graph_);

    avfilter_link(last_filter, 0, format_filter, 0);
    last_filter = format_filter;
  }

  // Finally, link the last filter with the buffersink
  avfilter_link(last_filter, 0, buffersink_ctx_, 0);

  // Configure graph
  if (int ret = avfilter_graph_config(filter_graph_, nullptr) < 0) {
    qCritical() << "Failed to configure graph:" << FFmpegError(ret);
    return false;
  }

  return true;
}

void FFmpegDecoder::FreeScaler()
{
  if (filter_graph_) {
    avfilter_graph_free(&filter_graph_);
    filter_graph_ = nullptr;
    buffersrc_ctx_ = nullptr;
    buffersink_ctx_ = nullptr;
  }
}

FramePtr FFmpegDecoder::GetFrameFromCache(const rational &t) const
{
  if (t < cached_frames_.front()->timestamp()) {

    if (cache_at_zero_) {
      return cached_frames_.front();
    }

  } else if (t > cached_frames_.back()->timestamp()) {

    if (cache_at_eof_) {
      return cached_frames_.back();
    }

  } else {

    // We already have this frame in the cache, find it
    for (auto it=cached_frames_.cbegin(); it!=cached_frames_.cend(); it++) {
      FramePtr this_frame = *it;

      auto next = it;
      next++;

      if (this_frame->timestamp() == t // Test for an exact match
          || (next != cached_frames_.cend() && (*next)->timestamp() > t)) { // Or for this frame to be the "closest"

        return this_frame;

      }
    }
  }

  return nullptr;
}

void FFmpegDecoder::RemoveFirstFrame()
{
  cached_frames_.pop_front();
  cache_at_zero_ = false;
}

VideoParams FFmpegDecoder::GetVideoParams() const
{
  return VideoParams(instance_.avstream()->codecpar->width,
                     instance_.avstream()->codecpar->height,
                     native_pix_fmt_,
                     native_channel_count_,
                     av_guess_sample_aspect_ratio(instance_.fmt_ctx(), instance_.avstream(), nullptr),
                     VideoParams::kInterlaceNone,
                     filter_params_.divider);
}

FFmpegDecoder::Instance::Instance() :
  fmt_ctx_(nullptr),
  codec_ctx_(nullptr),
  opts_(nullptr)
{
}

bool FFmpegDecoder::Instance::Open(const char *filename, int stream_index)
{
  // Open file in a format context
  int error_code = avformat_open_input(&fmt_ctx_, filename, nullptr, nullptr);

  // Handle format context error
  if (error_code != 0) {
    qCritical() << "Failed to open input:" << filename << FFmpegError(error_code);
    return false;
  }

  // Get stream information from format
  error_code = avformat_find_stream_info(fmt_ctx_, nullptr);

  // Handle get stream information error
  if (error_code < 0) {
    qCritical() << "Failed to find stream info:" << FFmpegError(error_code);
    return false;
  }

  // Get reference to correct AVStream
  avstream_ = fmt_ctx_->streams[stream_index];

  // Find decoder
  const AVCodec* codec = avcodec_find_decoder(avstream_->codecpar->codec_id);

  // Handle failure to find decoder
  if (codec == nullptr) {
    qCritical() << "Failed to find appropriate decoder for this codec:"
                << filename
                << stream_index
                << avstream_->codecpar->codec_id;
    return false;
  }

  // Allocate context for the decoder
  codec_ctx_ = avcodec_alloc_context3(codec);
  if (codec_ctx_ == nullptr) {
    qCritical() << "Failed to allocate codec context";
    return false;
  }

  // Copy parameters from the AVStream to the AVCodecContext
  error_code = avcodec_parameters_to_context(codec_ctx_, avstream_->codecpar);

  // Handle failure to copy parameters
  if (error_code < 0) {
    qCritical() << "Failed to copy parameters from AVStream to AVCodecContext";
    return false;
  }

  // Set multithreading setting
  error_code = av_dict_set(&opts_, "threads", "auto", 0);

  // Handle failure to set multithreaded decoding
  if (error_code < 0) {
    qCritical() << "Failed to set codec options, performance may suffer";
  }

  // Open codec
  error_code = avcodec_open2(codec_ctx_, codec, &opts_);
  if (error_code < 0) {
    char buf[512];
    av_strerror(error_code, buf, 512);
    qCritical() << "Failed to open codec" << codec->id << error_code << buf;
    return false;
  }

  return true;
}

void FFmpegDecoder::Instance::Close()
{
  if (opts_) {
    av_dict_free(&opts_);
    opts_ = nullptr;
  }

  if (codec_ctx_) {
    avcodec_free_context(&codec_ctx_);
    codec_ctx_ = nullptr;
  }

  if (fmt_ctx_) {
    avformat_close_input(&fmt_ctx_);
    fmt_ctx_ = nullptr;
  }
}

int FFmpegDecoder::Instance::GetFrame(AVPacket *pkt, AVFrame *frame)
{
  bool eof = false;

  int ret;

  // Clear any previous frames
  av_frame_unref(frame);

  while ((ret = avcodec_receive_frame(codec_ctx_, frame)) == AVERROR(EAGAIN) && !eof) {

    // Find next packet in the correct stream index
    ret = GetPacket(pkt);

    if (ret == AVERROR_EOF) {
      // Don't break so that receive gets called again, but don't try to read again
      eof = true;

      // Send a null packet to signal end of
      avcodec_send_packet(codec_ctx_, nullptr);
    } else if (ret < 0) {
      // Handle other error by breaking loop and returning the code we received
      break;
    } else {
      // Successful read, send the packet
      ret = avcodec_send_packet(codec_ctx_, pkt);

      // We don't need the packet anymore, so free it
      av_packet_unref(pkt);

      if (ret < 0) {
        break;
      }
    }
  }

  return ret;
}

const char *FFmpegDecoder::Instance::GetSubtitleHeader() const
{
  return reinterpret_cast<const char*>(codec_ctx_->subtitle_header);
}

int FFmpegDecoder::Instance::GetSubtitle(AVPacket *pkt, AVSubtitle *sub)
{
  int ret = GetPacket(pkt);

  if (ret >= 0) {
    int got_sub;
    ret = avcodec_decode_subtitle2(codec_ctx_, sub, &got_sub, pkt);
    if (!got_sub) {
      ret = -1;
    }
  }

  return ret;
}

int FFmpegDecoder::Instance::GetPacket(AVPacket *pkt)
{
  int ret;

  do {
    av_packet_unref(pkt);

    ret = av_read_frame(fmt_ctx_, pkt);
  } while (pkt->stream_index != avstream_->index && ret >= 0);

  return ret;
}

void FFmpegDecoder::Instance::Seek(int64_t timestamp)
{
  avcodec_flush_buffers(codec_ctx_);
  av_seek_frame(fmt_ctx_, avstream_->index, timestamp, AVSEEK_FLAG_BACKWARD);
}

}
