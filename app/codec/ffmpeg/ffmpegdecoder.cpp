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
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
}

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QtMath>

#include "codec/waveinput.h"
#include "common/define.h"
#include "common/filefunctions.h"
#include "common/functiontimer.h"
#include "common/timecodefunctions.h"
#include "ffmpegcommon.h"
#include "render/diskmanager.h"
#include "render/pixelservice.h"

FFmpegDecoder::FFmpegDecoder() :
  fmt_ctx_(nullptr),
  codec_ctx_(nullptr),
  scale_ctx_(nullptr),
  cache_at_zero_(false),
  cache_at_eof_(false),
  opts_(nullptr)
{
  // FIXME: Hardcoded, ideally this value is dynamically chosen based on memory restraints
  clear_timer_.setInterval(250);
  clear_timer_.moveToThread(qApp->thread());
  connect(&clear_timer_, &QTimer::timeout, this, &FFmpegDecoder::ClearTimerEvent);
}

FFmpegDecoder::~FFmpegDecoder()
{
  Close();
}

bool FFmpegDecoder::Open()
{
  QMutexLocker locker(&mutex_);

  if (open_) {
    return true;
  }

  if (!stream()) {
    Error(QStringLiteral("Tried to open a decoder with no footage stream set"));
    return false;
  }

  int error_code;

  // Convert QString to a C string
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
    Error(QStringLiteral("Failed to find appropriate decoder for this codec (%1:%2 - %3)")
          .arg(stream()->footage()->filename(),
               QString::number(avstream_->index),
               QString::number(avstream_->codecpar->codec_id)));
    return false;
  }

  // Allocate context for the decoder
  codec_ctx_ = avcodec_alloc_context3(codec);
  if (codec_ctx_ == nullptr) {
    Error(QStringLiteral("Failed to allocate codec context (%1 :: %2)").arg(stream()->footage()->filename(), stream()->index()));
    return false;
  }

  // Copy parameters from the AVStream to the AVCodecContext
  error_code = avcodec_parameters_to_context(codec_ctx_, avstream_->codecpar);

  // Handle failure to copy parameters
  if (error_code < 0) {
    FFmpegError(error_code);
    return false;
  }

  // Set multithreading setting
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

  if (avstream_->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
    // Get an Olive compatible AVPixelFormat
    ideal_pix_fmt_ = FFmpegCommon::GetCompatiblePixelFormat(static_cast<AVPixelFormat>(avstream_->codecpar->format));

    // Determine which Olive native pixel format we retrieved
    // Note that FFmpeg doesn't support float formats
    if (ideal_pix_fmt_ == AV_PIX_FMT_RGBA) {
      native_pix_fmt_ = PixelFormat::PIX_FMT_RGBA8;
    } else if (ideal_pix_fmt_ == AV_PIX_FMT_RGBA64) {
      native_pix_fmt_ = PixelFormat::PIX_FMT_RGBA16U;
    } else {
      // We should never get here, but just in case...
      qFatal("Invalid output format");
    }

    scale_ctx_ = sws_getContext(avstream_->codecpar->width,
                                avstream_->codecpar->height,
                                static_cast<AVPixelFormat>(avstream_->codecpar->format),
                                avstream_->codecpar->width,
                                avstream_->codecpar->height,
                                ideal_pix_fmt_,
                                0,
                                nullptr,
                                nullptr,
                                nullptr);

    if (!scale_ctx_) {
      Error(QStringLiteral("Failed to allocate SwsContext"));
      return false;
    }

    second_ts_ = qRound64(av_q2d(av_inv_q(avstream_->time_base)));

    QMetaObject::invokeMethod(&clear_timer_, "start");
  }

  // All allocation succeeded so we set the state to open
  open_ = true;

  return true;
}

Decoder::RetrieveState FFmpegDecoder::GetRetrieveState(const rational& time)
{
  QMutexLocker locker(&mutex_);

  if (!open_) {
    return kFailedToOpen;
  }

  if (avstream_->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {

  } else if (avstream_->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
    AudioStreamPtr audio_stream = std::static_pointer_cast<AudioStream>(stream());

    if (time > audio_stream->index_length() && !audio_stream->index_done()) {
      return kIndexUnavailable;
    }
  }

  return kReady;
}

FramePtr FFmpegDecoder::RetrieveVideo(const rational &timecode)
{
  QMutexLocker locker(&mutex_);

  if (!open_) {
    qWarning() << "Tried to retrieve video on a decoder that's still closed";
    return nullptr;
  }

  if (avstream_->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
    return nullptr;
  }

  int64_t target_ts = Timecode::time_to_timestamp(timecode, avstream_->time_base) + avstream_->start_time;

  Frame* return_frame = nullptr;

  // See if our RAM cache already has a frame that matches this timestamp
  if (!cached_frames_.isEmpty()) {

    if (target_ts < cached_frames_.first()->native_timestamp()) {

      if (cache_at_zero_) {
        return_frame = cached_frames_.first();
        cached_frames_.accessedFirst();
      }

    } else if (target_ts > cached_frames_.last()->native_timestamp()) {

      if (cache_at_eof_) {
        return_frame = cached_frames_.last();
        cached_frames_.accessedLast();
      }

    } else {

      // We already have this frame in the cache, find it
      for (int i=0;i<cached_frames_.size();i++) {
        Frame* this_frame = cached_frames_.at(i);

        if (this_frame->native_timestamp() == target_ts // Test for an exact match
            || (i < cached_frames_.size() - 1 && cached_frames_.at(i+1)->native_timestamp() > target_ts)) { // Or for this frame to be the "closest"

          return_frame = this_frame;
          cached_frames_.accessed(i);

          break;

        }
      }
    }
  }

  // See if we stored this frame in the disk cache
  /*
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
  */

  // If we have no disk cache, we'll need to find this frame ourselves
  if (!return_frame) {
    int64_t seek_ts = target_ts;
    bool still_seeking = false;

    // If the frame wasn't in the frame cache, see if this frame cache is too old to use
    if (cached_frames_.isEmpty()
        || target_ts < cached_frames_.first()->native_timestamp()
        || target_ts > cached_frames_.last()->native_timestamp() + 2*second_ts_) {
      ClearFrameCache();

      Seek(seek_ts);
      if (seek_ts == 0) {
        cache_at_zero_ = true;
      }

      still_seeking = true;
    }

    int ret;
    AVPacket* pkt = av_packet_alloc();
    AVFrame* working_frame = av_frame_alloc();

    while (true) {
      // Allocate a new frame

      // Pull from the decoder
      ret = GetFrame(pkt, working_frame);

      // Handle any errors that aren't EOF (EOF is handled later on)
      if (ret < 0 && ret != AVERROR_EOF) {
        FFmpegError(ret);
        break;
      }

      if (still_seeking) {
        // Handle a failure to seek (occurs on some media)
        // We'll only be here if the frame cache was emptied earlier
        if (!cache_at_zero_ && (ret == AVERROR_EOF || working_frame->pts > target_ts)) {

          seek_ts = qMax(static_cast<int64_t>(0), seek_ts - second_ts_);
          Seek(seek_ts);
          if (seek_ts == 0) {
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
        return_frame = cached_frames_.last();

      } else {

        bool working_frame_is_the_one = false;

        // If this is a valid frame, see if this or the frame before it are the one we need
        if (working_frame->pts == target_ts) {
          working_frame_is_the_one = true;
        } else if (working_frame->pts > target_ts) {
          if (cached_frames_.isEmpty() && cache_at_zero_) {
            working_frame_is_the_one = true;
          } else {
            return_frame = cached_frames_.last();
          }
        }

        // Whatever it is, keep this frame in memory for the time being just in case
        Frame* working_frame_converted = cached_frames_.append(VideoRenderingParams(avstream_->codecpar->width,
                                                                                    avstream_->codecpar->height,
                                                                                    avstream_->time_base,
                                                                                    native_pix_fmt_,
                                                                                    RenderMode::kOffline));

        working_frame_converted->set_timestamp(Timecode::timestamp_to_time(target_ts, avstream_->time_base));
        working_frame_converted->set_sample_aspect_ratio(av_guess_sample_aspect_ratio(fmt_ctx_, avstream_, nullptr));
        working_frame_converted->set_native_timestamp(working_frame->pts);

        // Convert frame to RGBA for the rest of the pipeline
        uint8_t* output_data = reinterpret_cast<uint8_t*>(working_frame_converted->data());
        int output_linesize = working_frame_converted->width() * kRGBAChannels * PixelService::BytesPerChannel(native_pix_fmt_);

        sws_scale(scale_ctx_,
                  working_frame->data,
                  working_frame->linesize,
                  0,
                  avstream_->codecpar->height,
                  &output_data,
                  &output_linesize);

        if (working_frame_is_the_one) {
          // We found the frame we want
          return_frame = working_frame_converted;
        }

        if (return_frame) {
          break;
        }
      }
    }

    av_packet_free(&pkt);
    av_frame_free(&working_frame);
  }

  // We found the frame, we'll return a copy
  if (return_frame) {
    FramePtr copy = Frame::Create();
    copy->set_width(return_frame->width());
    copy->set_height(return_frame->height());
    copy->set_format(return_frame->format());
    copy->set_timestamp(return_frame->timestamp());
    copy->set_sample_aspect_ratio(return_frame->sample_aspect_ratio());
    copy->allocate();

    memcpy(copy->data(), return_frame->data(), copy->allocated_size());

    return copy;
  }

  return nullptr;
}

FramePtr FFmpegDecoder::RetrieveAudio(const rational &timecode, const rational &length, const AudioRenderingParams &params)
{
  QMutexLocker locker(&mutex_);

  if (!open_) {
    qWarning() << "Tried to retrieve audio on a decoder that's still closed";
    return nullptr;
  }

  if (avstream_->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
    return nullptr;
  }

  //Conform(params, cancelled);

  WaveInput input(GetConformedFilename(params));

  if (input.open()) {
    const AudioRenderingParams& input_params = input.params();

    FramePtr audio_frame = Frame::Create();
    audio_frame->set_audio_params(input_params);
    audio_frame->set_sample_count(input_params.time_to_samples(length));
    audio_frame->allocate();

    input.read(input_params.time_to_bytes(timecode),
               audio_frame->data(),
               audio_frame->allocated_size());

    input.close();

    return audio_frame;
  }

  return nullptr;
}

void FFmpegDecoder::Close()
{
  QMutexLocker locker(&mutex_);

  ClearResources();

  clear_timer_.stop();
}

QString FFmpegDecoder::id()
{
  return QStringLiteral("ffmpeg");
}

bool FFmpegDecoder::SupportsVideo()
{
  return true;
}

bool FFmpegDecoder::SupportsAudio()
{
  return true;
}

bool FFmpegDecoder::Probe(Footage *f, const QAtomicInt* cancelled)
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

  QList<Stream*> streams_that_need_manual_duration;

  // Handle format context error
  if (error_code == 0) {

    // Retrieve metadata about the media
    av_dump_format(fmt_ctx_, 0, filename, 0);

    // Dump it into the Footage object
    for (unsigned int i=0;i<fmt_ctx_->nb_streams;i++) {

      avstream_ = fmt_ctx_->streams[i];

      StreamPtr str;

      if (avstream_->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {

        // Create a video stream object
        VideoStreamPtr video_stream = std::make_shared<VideoStream>();

        video_stream->set_width(avstream_->codecpar->width);
        video_stream->set_height(avstream_->codecpar->height);
        video_stream->set_frame_rate(av_guess_frame_rate(fmt_ctx_, avstream_, nullptr));
        video_stream->set_start_time(avstream_->start_time);

        str = video_stream;

      } else if (avstream_->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {

        // Create an audio stream object
        AudioStreamPtr audio_stream = std::make_shared<AudioStream>();

        uint64_t channel_layout = avstream_->codecpar->channel_layout;
        if (!channel_layout) {
          channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(avstream_->codecpar->channels));
        }

        audio_stream->set_channel_layout(channel_layout);
        audio_stream->set_channels(avstream_->codecpar->channels);
        audio_stream->set_sample_rate(avstream_->codecpar->sample_rate);

        str = audio_stream;

      } else {

        // This is data we can't utilize at the moment, but we make a Stream object anyway to keep parity with the file
        str = std::make_shared<Stream>();

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
        default:
          // We should never realistically get here, but we make an "invalid" stream just in case
          str->set_type(Stream::kUnknown);
          break;
        }

      }

      str->set_index(avstream_->index);
      str->set_timebase(avstream_->time_base);
      str->set_duration(avstream_->duration);

      // The container/stream info may not contain a duration, so we'll need to manually retrieve it
      if (avstream_->duration == AV_NOPTS_VALUE) {
        streams_that_need_manual_duration.append(str.get());
      }

      f->add_stream(str);
    }

    // As long as we can open the container and retrieve information, this was a successful probe
    result = true;
  }

  // If the metadata did not contain a duration, we'll need to loop through the file to retrieve it
  if (!streams_that_need_manual_duration.isEmpty()) {

    AVPacket* pkt = av_packet_alloc();

    QVector<int64_t> durations(streams_that_need_manual_duration.size());
    durations.fill(0);

    while (true) {
      if (cancelled && *cancelled) {
        break;
      }

      // Ensure previous buffers are cleared
      av_packet_unref(pkt);

      // Read packet from file
      int ret = av_read_frame(fmt_ctx_, pkt);

      if (ret < 0) {
        // Handle errors that aren't EOF (which simply means the file is finished)
        if (ret != AVERROR_EOF) {
          qWarning() << "Error while finding duration";
        }
        break;
      } else {
        for (int i=0;i<streams_that_need_manual_duration.size();i++) {
          if (streams_that_need_manual_duration.at(i)->index() == pkt->stream_index
              && pkt->pts > durations.at(i)) {
            durations.replace(i, pkt->pts);
          }
        }
      }
    }

    av_packet_free(&pkt);

    if (!cancelled || !*cancelled) {
      for (int i=0;i<streams_that_need_manual_duration.size();i++) {
        streams_that_need_manual_duration.at(i)->set_duration(durations.at(i));
      }
    }

  }

  // Free all memory
  Close();

  return result;
}

void FFmpegDecoder::FFmpegError(int error_code)
{
  char err[1024];
  av_strerror(error_code, err, 1024);

  Error(QStringLiteral("Error decoding %1 - %2 %3").arg(stream()->footage()->filename(),
                                                        QString::number(error_code),
                                                        err));
}

void FFmpegDecoder::Error(const QString &s)
{
  qWarning() << s;

  ClearResources();
}

void FFmpegDecoder::Index(const QAtomicInt* cancelled)
{
  if (!open_) {
    qWarning() << "Indexing function tried to run while decoder was closed";
    return;
  }

  QMutexLocker locker(stream()->index_process_lock());

  if (stream()->type() == Stream::kAudio) {

    if (QFileInfo::exists(GetIndexFilename())) {
      WaveInput input(GetIndexFilename());
      if (input.open()) {
        std::static_pointer_cast<AudioStream>(stream())->set_index_done(true);
        std::static_pointer_cast<AudioStream>(stream())->set_index_length(input.params().bytes_to_time(input.data_length()));

        input.close();
      }
    } else {
      UnconditionalAudioIndex(cancelled);
    }

  }
}

QString FFmpegDecoder::GetIndexFilename()
{
  if (!open_) {
    qWarning() << "GetIndexFilename tried to run while decoder was closed";
    return QString();
  }

  return GetMediaIndexFilename(GetUniqueFileIdentifier(stream()->footage()->filename()))
      .append(QString::number(avstream_->index));
}

void FFmpegDecoder::UnconditionalAudioIndex(const QAtomicInt* cancelled)
{
  // Iterate through each audio frame and extract the PCM data

  Seek(0);

  uint64_t channel_layout = avstream_->codecpar->channel_layout;
  if (!channel_layout) {
    if (!avstream_->codecpar->channels) {
      // No channel data - we can't do anything with this
      return;
    }

    channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(avstream_->codecpar->channels));
  }

  AudioStreamPtr audio_stream = std::static_pointer_cast<AudioStream>(stream());

  // This should be unnecessary, but just in case...
  audio_stream->clear_index();

  SwrContext* resampler = nullptr;
  AVSampleFormat src_sample_fmt = static_cast<AVSampleFormat>(avstream_->codecpar->format);
  AVSampleFormat dst_sample_fmt;

  // We don't use planar types internally, so if this is a planar format convert it now
  if (av_sample_fmt_is_planar(src_sample_fmt)) {
    dst_sample_fmt = av_get_packed_sample_fmt(src_sample_fmt);

    resampler = swr_alloc_set_opts(nullptr,
                                   static_cast<int64_t>(avstream_->codecpar->channel_layout),
                                   dst_sample_fmt,
                                   avstream_->codecpar->sample_rate,
                                   static_cast<int64_t>(avstream_->codecpar->channel_layout),
                                   src_sample_fmt,
                                   avstream_->codecpar->sample_rate,
                                   0,
                                   nullptr);
  } else {
    dst_sample_fmt = src_sample_fmt;
  }

  AudioRenderingParams wave_params(avstream_->codecpar->sample_rate,
                                   channel_layout,
                                   FFmpegCommon::GetNativeSampleFormat(dst_sample_fmt));
  WaveOutput wave_out(GetIndexFilename(), wave_params);

  AVPacket* pkt = av_packet_alloc();
  AVFrame* frame = av_frame_alloc();
  int ret;

  if (wave_out.open()) {
    while (true) {
      // Check if we have a `cancelled` ptr and its value
      if (cancelled && *cancelled) {
        break;
      }

      ret = GetFrame(pkt, frame);

      if (ret < 0) {
        break;
      } else {
        AVFrame* data_frame;

        if (resampler != nullptr) {
          // We must need to resample this (mainly just convert from planar to packed if necessary)
          data_frame = av_frame_alloc();
          data_frame->sample_rate = frame->sample_rate;
          data_frame->channel_layout = frame->channel_layout;
          data_frame->channels = frame->channels;
          data_frame->format = dst_sample_fmt;
          av_frame_make_writable(data_frame);

          ret = swr_convert_frame(resampler, data_frame, frame);

          if (ret != 0) {
            char err_str[50];
            av_strerror(ret, err_str, 50);
            qWarning() << "libswresample failed with error:" << ret << err_str;
          }
        } else {
          // No resampling required, we can write directly from te frame buffer
          data_frame = frame;
        }

        int buffer_sz = av_samples_get_buffer_size(nullptr,
                                                   avstream_->codecpar->channels,
                                                   data_frame->nb_samples,
                                                   dst_sample_fmt,
                                                   0); // FIXME: Documentation unclear - should this be 0 or 1?

        // Write packed WAV data to the disk cache
        wave_out.write(reinterpret_cast<char*>(data_frame->data[0]), buffer_sz);

        audio_stream->set_index_length(wave_params.bytes_to_time(wave_out.data_length()));

        // If we allocated an output for the resampler, delete it here
        if (data_frame != frame) {
          av_frame_free(&data_frame);
        }

        SignalIndexProgress(frame->pts);
      }
    }

    wave_out.close();

    if (cancelled && *cancelled) {
      // Audio index didn't complete, delete it
      QFile(GetIndexFilename()).remove();
      audio_stream->clear_index();
    } else {
      audio_stream->set_index_done(true);
    }
  } else {
    qWarning() << "Failed to open WAVE output for indexing";
  }

  if (resampler != nullptr) {
    swr_free(&resampler);
  }

  av_frame_free(&frame);
  av_packet_free(&pkt);

  Seek(0);
}

int FFmpegDecoder::GetFrame(AVPacket *pkt, AVFrame *frame)
{
  bool eof = false;

  int ret;

  // Clear any previous frames
  av_frame_unref(frame);

  while ((ret = avcodec_receive_frame(codec_ctx_, frame)) == AVERROR(EAGAIN) && !eof) {

    // Find next packet in the correct stream index
    do {
      // Free buffer in packet if there is one
      av_packet_unref(pkt);

      // Read packet from file
      ret = av_read_frame(fmt_ctx_, pkt);
    } while (pkt->stream_index != avstream_->index && ret >= 0);

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

void FFmpegDecoder::Seek(int64_t timestamp)
{
  avcodec_flush_buffers(codec_ctx_);
  av_seek_frame(fmt_ctx_, avstream_->index, timestamp, AVSEEK_FLAG_BACKWARD);
}

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
}

/*void FFmpegDecoder::RemoveFirstFromFrameCache()
{
  if (cached_frames_.isEmpty()) {
    return;
  }

  AVFrame* first = cached_frames_.takeFirst();
  av_frame_free(&first);
  cache_at_zero_ = false;
}

void FFmpegDecoder::RemoveLastFromFrameCache()
{
  if (cached_frames_.isEmpty()) {
    return;
  }

  AVFrame* last = cached_frames_.takeLast();
  av_frame_free(&last);
  cache_at_eof_ = false;
}*/

void FFmpegDecoder::ClearFrameCache()
{
  cached_frames_.clear();
  cache_at_eof_ = false;
  cache_at_zero_ = false;
}

void FFmpegDecoder::ClearResources()
{
  if (opts_) {
    av_dict_free(&opts_);
    opts_ = nullptr;
  }

  ClearFrameCache();

  if (scale_ctx_) {
    sws_freeContext(scale_ctx_);
    scale_ctx_ = nullptr;
  }

  if (codec_ctx_) {
    avcodec_free_context(&codec_ctx_);
    codec_ctx_ = nullptr;
  }

  if (fmt_ctx_) {
    avformat_close_input(&fmt_ctx_);
    fmt_ctx_ = nullptr;
  }

  open_ = false;
}

void FFmpegDecoder::ClearTimerEvent()
{
  QMutexLocker locker(&mutex_);

  cache_at_zero_ = false;
  cached_frames_.remove_old_frames(QDateTime::currentMSecsSinceEpoch() - clear_timer_.interval());
}
