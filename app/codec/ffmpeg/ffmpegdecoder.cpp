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

#include <OpenImageIO/imagebuf.h>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QtMath>
#include <QThread>
#include <QtConcurrent/QtConcurrent>

#include "codec/waveinput.h"
#include "common/define.h"
#include "common/filefunctions.h"
#include "common/functiontimer.h"
#include "common/timecodefunctions.h"
#include "ffmpegcommon.h"
#include "render/framehashcache.h"
#include "render/diskmanager.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

QHash< Stream*, QList<FFmpegDecoderInstance*> > FFmpegDecoder::instance_map_;
QMutex FFmpegDecoder::instance_map_lock_;
QHash< FFmpegDecoder::FFmpegFramePoolKey, FFmpegDecoder::FFmpegFramePoolValue > FFmpegDecoder::frame_pool_map_;

// FIXME: Hardcoded value. It seems to work fine, but is there a possibility we should make
//        this a dynamic value somehow or a configurable value?
const int FFmpegDecoderInstance::kMaxFrameLife = 2000;

FFmpegDecoder::FFmpegDecoder() :
  scale_ctx_(nullptr),
  scale_divider_(0)
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

  Q_ASSERT(stream());

  // Convert QString to a C string
  QByteArray fn_bytes = stream()->footage()->filename().toUtf8();

  FFmpegDecoderInstance* our_instance = new FFmpegDecoderInstance(fn_bytes.constData(), stream()->index());

  if (!our_instance->IsValid()) {
    delete our_instance;
    return false;
  }

  if (stream()->type() == Stream::kVideo) {
    // Get an Olive compatible AVPixelFormat
    src_pix_fmt_ = static_cast<AVPixelFormat>(our_instance->stream()->codecpar->format);
    ideal_pix_fmt_ = FFmpegCommon::GetCompatiblePixelFormat(src_pix_fmt_);

    // Determine which Olive native pixel format we retrieved
    // Note that FFmpeg doesn't support float formats
    native_pix_fmt_ = GetNativePixelFormat(ideal_pix_fmt_);

    Q_ASSERT(native_pix_fmt_ != PixelFormat::PIX_FMT_INVALID);
  }

  if (StreamUsesMultipleInstances(stream())) {
    // Video optimizes with multiple instances that we can swap between
    QMutexLocker map_locker(&instance_map_lock_);

    VideoStreamPtr vs = std::static_pointer_cast<VideoStream>(stream());

    FFmpegFramePoolKey key = {vs->width(), vs->height(), src_pix_fmt_};
    FFmpegFramePoolValue& frame_pool = frame_pool_map_[key];

    if (!frame_pool.pool) {
      // Frames are allocated as threads * threads, to scale from each thread sharing one set
      // to all of them working individually
      int thread_count = QThread::idealThreadCount();
      int max_memory_frame_count = thread_count * thread_count;
      frame_pool.pool = new FFmpegFramePool(max_memory_frame_count);
    }
    frame_pool.handles++;

    our_instance->SetFramePool(frame_pool.pool);

    instance_map_[stream().get()].append(our_instance);
  } else {
    // Images, image sequences, and audio don't need an instance
    delete our_instance;
  }

  // All allocation succeeded so we set the state to open
  open_ = true;

  return true;
}

FramePtr FFmpegDecoder::RetrieveStillImage(const rational &timecode, const int &divider)
{
  // This is a still image
  VideoStreamPtr is = std::static_pointer_cast<VideoStream>(stream());

  QString img_filename = stream()->footage()->filename();

  // If it's an image sequence, we'll probably need to transform the filename
  if (is->video_type() == VideoStream::kVideoTypeImageSequence) {
    int64_t ts = std::static_pointer_cast<VideoStream>(stream())->get_time_in_timebase_units(timecode);

    img_filename = TransformImageSequenceFileName(stream()->footage()->filename(), ts);
  }

  FFmpegDecoderInstance i(img_filename.toUtf8(), stream()->index());

  AVPacket* pkt = av_packet_alloc();
  AVFrame* frame = av_frame_alloc();
  FramePtr output_frame = nullptr;

  int ret = i.GetFrame(pkt, frame);

  if (ret >= 0) {
    output_frame = BuffersToNativeFrame(divider,
                                        is->width(),
                                        is->height(),
                                        0,
                                        frame->data,
                                        frame->linesize);
  } else {
    qWarning() << "Failed to retrieve still image from decoder";
  }

  av_frame_free(&frame);
  av_packet_free(&pkt);

  return output_frame;
}

FramePtr FFmpegDecoder::RetrieveVideo(const rational &timecode, const int &divider)
{
  if (!open_) {
    qWarning() << "Tried to retrieve video on a decoder that's still closed";
    return nullptr;
  }

  if (stream()->type() != Stream::kVideo) {
    return nullptr;
  }

  VideoStreamPtr vs = std::static_pointer_cast<VideoStream>(stream());

  if (vs->video_type() == VideoStream::kVideoTypeStill
      || vs->video_type() == VideoStream::kVideoTypeImageSequence) {

    return RetrieveStillImage(timecode, divider);

  } else {

    FFmpegFramePool::ElementPtr return_frame = nullptr;

    int64_t target_ts = vs->get_time_in_timebase_units(timecode);

    FFmpegDecoderInstance* working_instance = nullptr;

    int divided_width = VideoParams::GetScaledDimension(vs->width(), divider);
    int divided_height = VideoParams::GetScaledDimension(vs->height(), divider);

    // Find instance
    do {
      QMutexLocker list_locker(&instance_map_lock_);

      const QList<FFmpegDecoderInstance*>& instances = instance_map_.value(stream().get());

      FFmpegFramePool* pool = frame_pool_map_.value({vs->width(), vs->height(), src_pix_fmt_}).pool;

      if (pool->width() != divided_width || pool->height() != divided_height) {
        // Clear all instance queues
        foreach (FFmpegDecoderInstance* i, instances) {
          i->ClearFrameCache();
        }

        // Set new frame pool parameters
        pool->SetParameters(divided_width, divided_height, src_pix_fmt_);
      }

      QList<FFmpegDecoderInstance*> non_ideal_contenders;

      foreach (FFmpegDecoderInstance* i, instances) {

        i->cache_lock()->lock();

        if (i->CacheContainsTime(target_ts)) {

          // Found our instance, allow others to enter the list

          list_locker.unlock();

          // Get the frame from this cache
          return_frame = i->GetFrameFromCache(target_ts);

          // Got our frame, allow cache to continue
          i->cache_lock()->unlock();
          break;

        } else if (i->CacheWillContainTime(target_ts) || i->CacheCouldContainTime(target_ts)) {

          // Found our instance, allow others to enter the list
          list_locker.unlock();

          // If the instance is currently in use, enter into a loop of seeing from frames come up next in case one is ours
          if (i->IsWorking()) {

            do {
              // Allow instance to continue to the next frame
              i->cache_wait_cond()->wait(i->cache_lock());

              // See if the cache now contains this frame, if so we'll exit this loop
              if (i->CacheContainsTime(target_ts)) {

                // Grab the frame
                return_frame = i->GetFrameFromCache(target_ts);

                // We can release this worker now since we don't need it anymore
                i->cache_lock()->unlock();

              } else if (!i->IsWorking()) {

                // This instance finished and we didn't get our frame, we'll take it and continue it
                working_instance = i;
                break;

              }
            } while (!return_frame);

          } else {
            // Otherwise, we'll grab this instance and continue it ourselves
            working_instance = i;
          }

          break;

        } else if (i->IsWorking()) {

          // Ignore currently working instances
          i->cache_lock()->unlock();

        } else if (i->CacheIsEmpty()) {

          // Prioritize this cache over others (leaves this instance LOCKED in case we end up using it later)
          non_ideal_contenders.prepend(i);

        } else {

          // De-prioritize this cache (leaves this instance LOCKED in case we end up using it later)
          non_ideal_contenders.append(i);

        }
      }

      // If we didn't find a suitable contender, grab the first non-suitable and roll with that
      if (!return_frame && !working_instance && !non_ideal_contenders.isEmpty()) {
        working_instance = non_ideal_contenders.takeFirst();
      }

      // For all instances we left locked but didn't end up using, lock them now
      foreach (FFmpegDecoderInstance* unsuitable_instance, non_ideal_contenders) {
        unsuitable_instance->cache_lock()->unlock();
      }
    } while (!return_frame && !working_instance);

    if (!return_frame && working_instance) {

      // This instance SHOULD remain locked from our earlier loop, making this operation safe
      working_instance->SetWorking(true);

      // Retrieve frame
      return_frame = working_instance->RetrieveFrame(target_ts, divider, true);

      // Set working to false and wake any threads waiting
      working_instance->cache_lock()->lock();
      working_instance->SetWorking(false);
      working_instance->cache_wait_cond()->wakeAll();
      working_instance->cache_lock()->unlock();
    }

    // We found the frame, we'll return a copy
    if (return_frame) {
      // Align buffer to data/linesize points that can be passed to sws_scale
      uint8_t* input_data[4];
      int input_linesize[4];

      av_image_fill_arrays(input_data,
                           input_linesize,
                           reinterpret_cast<const uint8_t*>(return_frame->data()),
                           src_pix_fmt_,
                           divided_width,
                           divided_height,
                           1);

      return BuffersToNativeFrame(divider,
                                  vs->width(),
                                  vs->height(),
                                  timecode,
                                  input_data,
                                  input_linesize);
    }

  }

  return nullptr;
}

SampleBufferPtr FFmpegDecoder::RetrieveAudio(const rational &timecode, const rational &length, const AudioParams &params)
{
  if (!open_) {
    qWarning() << "Tried to retrieve audio on a decoder that's still closed";
    return nullptr;
  }

  if (stream()->type() != Stream::kAudio) {
    return nullptr;
  }

  QString wav_fn = GetConformedFilename(params);
  WaveInput input(wav_fn);

  if (input.open()) {
    const AudioParams& input_params = input.params();

    // Read bytes from wav
    QByteArray packed_data = input.read(input_params.time_to_bytes(timecode), input_params.time_to_bytes(length));
    input.close();

    // Create sample buffer
    SampleBufferPtr sample_buffer = SampleBuffer::CreateFromPackedData(input_params, packed_data);

    return sample_buffer;
  }

  qCritical() << "Failed to open cached file" << wav_fn;

  return nullptr;
}

void FFmpegDecoder::Close()
{
  if (stream() && StreamUsesMultipleInstances(stream())) {
    // Clear whichever instance is not in use and is least useful (there are only ever as many instances as there are
    // threads so if this thread is closing, an instance MUST be inactive)
    QMutexLocker l(&instance_map_lock_);

    QList<FFmpegDecoderInstance*> list = instance_map_.value(stream().get());

    if (!list.isEmpty()) {
      // Rank the instances by least useful (the top one should be one that isn't working and isn't in use)
      QList<FFmpegDecoderInstance*> least_useful;

      foreach (FFmpegDecoderInstance* i, list) {
        i->cache_lock()->lock();

        if (i->IsWorking()) {
          // Don't bother any currently working instances
          i->cache_lock()->unlock();
          continue;
        }

        if (i->CacheIsEmpty()) {
          least_useful.prepend(i);
        } else {
          least_useful.append(i);
        }
      }

      // Remove the least useful from the list and re-insert it into the map
      FFmpegDecoderInstance* least_useful_instance = least_useful.first();
      list.removeOne(least_useful_instance);
      instance_map_.insert(stream().get(), list);

      // If there are no more instances, destroy frame pool
      VideoStreamPtr vs = std::static_pointer_cast<VideoStream>(stream());
      FFmpegFramePoolKey frame_pool_key = {vs->width(), vs->height(), src_pix_fmt_};
      FFmpegFramePoolValue& frame_pool_info = frame_pool_map_[frame_pool_key];
      frame_pool_info.handles--;

      if (frame_pool_info.handles == 0) {
        delete frame_pool_info.pool;
        frame_pool_map_.remove(frame_pool_key);
      }

      // We're done with the list now, we can unlock it and allow others to use it
      l.unlock();

      // Unlock all the instances we locked
      foreach (FFmpegDecoderInstance* i, least_useful) {
        i->cache_lock()->unlock();
      }

      // Delete this least useful instance now that we've definitely taken ownership of it
      least_useful_instance->deleteLater();
    }
  }

  ClearResources();
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

FootagePtr FFmpegDecoder::Probe(const QString& filename, const QAtomicInt* cancelled) const
{
  // Variable for receiving errors from FFmpeg
  int error_code;

  // Result to return
  FootagePtr footage = nullptr;

  // Convert QString to a C string
  QByteArray ba = filename.toUtf8();
  const char* filename_c = ba.constData();

  // Open file in a format context
  AVFormatContext* fmt_ctx = nullptr;
  error_code = avformat_open_input(&fmt_ctx, filename_c, nullptr, nullptr);

  // Handle format context error
  if (error_code == 0) {

    // Retrieve metadata about the media
    avformat_find_stream_info(fmt_ctx, nullptr);

    QVector<StreamPtr> streams(fmt_ctx->nb_streams);

    // Dump it into the Footage object
    for (unsigned int i=0;i<fmt_ctx->nb_streams;i++) {

      AVStream* avstream = fmt_ctx->streams[i];

      // Find decoder for this stream, if it exists we can proceed
      AVCodec* decoder = avcodec_find_decoder(avstream->codecpar->codec_id);

      StreamPtr str;

      if (avstream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && decoder) {

        bool image_is_still = false;
        rational pixel_aspect_ratio;
        rational frame_rate;
        VideoParams::Interlacing interlacing = VideoParams::kInterlaceNone;

        {
          // Read at least two frames to get more information about this video stream
          AVPacket* pkt = av_packet_alloc();
          AVFrame* frame = av_frame_alloc();

          {
            FFmpegDecoderInstance instance(filename_c, i);

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
                                                                instance.stream(),
                                                                frame);

              frame_rate = av_guess_frame_rate(instance.fmt_ctx(),
                                               instance.stream(),
                                               frame);
            }

            // Read second frame
            int ret = instance.GetFrame(pkt, frame);

            if (ret >= 0) {
              // Check if we need a manual duration
              if (avstream->duration == AV_NOPTS_VALUE) {
                int64_t new_dur;

                do {
                  new_dur = frame->pts;
                } while (instance.GetFrame(pkt, frame) >= 0);

                avstream->duration = new_dur;
              }
            } else if (ret == AVERROR_EOF) {
              // Video has only one frame in it, treat it like a still image
              image_is_still = true;
            }
          }

          av_frame_free(&frame);
          av_packet_free(&pkt);
        }

        VideoStreamPtr video_stream = std::make_shared<VideoStream>();

        if (image_is_still) {
          video_stream->set_video_type(VideoStream::kVideoTypeStill);
        } else {
          video_stream->set_video_type(VideoStream::kVideoTypeVideo);

          video_stream->set_frame_rate(frame_rate);
          video_stream->set_start_time(avstream->start_time);
        }

        video_stream->set_width(avstream->codecpar->width);
        video_stream->set_height(avstream->codecpar->height);
        video_stream->set_format(GetNativePixelFormat(FFmpegCommon::GetCompatiblePixelFormat(static_cast<AVPixelFormat>(avstream->codecpar->format))));
        video_stream->set_interlacing(interlacing);
        video_stream->set_pixel_aspect_ratio(pixel_aspect_ratio);

        str = video_stream;

      } else if (avstream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && decoder) {

        // Create an audio stream object
        AudioStreamPtr audio_stream = std::make_shared<AudioStream>();

        uint64_t channel_layout = avstream->codecpar->channel_layout;
        if (!channel_layout) {
          channel_layout = static_cast<uint64_t>(av_get_default_channel_layout(avstream->codecpar->channels));
        }

        audio_stream->set_channel_layout(channel_layout);
        audio_stream->set_channels(avstream->codecpar->channels);
        audio_stream->set_sample_rate(avstream->codecpar->sample_rate);

        if (avstream->duration == AV_NOPTS_VALUE) {
          // Loop through stream until we get the whole duration
          FFmpegDecoderInstance instance(filename_c, i);

          AVPacket* pkt = av_packet_alloc();
          AVFrame* frame = av_frame_alloc();

          int64_t new_dur;

          do {
            new_dur = frame->pts;
          } while (instance.GetFrame(pkt, frame) >= 0);

          avstream->duration = new_dur;

          av_frame_free(&frame);
          av_packet_free(&pkt);
        }

        str = audio_stream;

      } else {

        // This is data we can't utilize at the moment, but we make a Stream object anyway to keep parity with the file
        str = std::make_shared<Stream>();

        // Set the correct codec type based on FFmpeg's result
        switch (avstream->codecpar->codec_type) {
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
          // Fallback to an unknown stream
          str->set_type(Stream::kUnknown);
          break;
        }

      }

      str->set_index(avstream->index);
      str->set_timebase(avstream->time_base);
      str->set_duration(avstream->duration);

      streams[i] = str;
    }

    // Check if we could pick up any streams in this file
    bool found_valid_streams = false;

    foreach (StreamPtr stream, streams) {
      if (stream->type() != Stream::kUnknown) {
        found_valid_streams = true;
        break;
      }
    }

    if (found_valid_streams) {
      // We actually have footage we can return instead of nullptr
      footage = std::make_shared<Footage>();

      // Copy streams over
      foreach (StreamPtr stream, streams) {
        footage->add_stream(stream);
      }
    }
  }

  // Free all memory
  avformat_close_input(&fmt_ctx);

  return footage;
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

bool FFmpegDecoder::ConformAudio(const QAtomicInt *cancelled, const AudioParams &p)
{
  // Iterate through each audio frame and extract the PCM data
  AudioStreamPtr audio_stream = std::static_pointer_cast<AudioStream>(stream());

  // Check if we already have a conform of this type
  QString conformed_fn = GetConformedFilename(p);

  if (QFileInfo::exists(conformed_fn)) {

    // If we have one, and we can open it correctly, we can use it as-is
    WaveInput input(conformed_fn);
    if (input.open()) {
      audio_stream->append_conformed_version(p);

      input.close();

      return true;
    }
  }

  // Conform doesn't exist, we'll have to produce one
  FFmpegDecoderInstance index_instance(stream()->footage()->filename().toUtf8(),
                                       stream()->index());

  // Handle NULL channel layout
  uint64_t channel_layout = ValidateChannelLayout(index_instance.stream());
  if (!channel_layout) {
    qCritical() << "Failed to determine channel layout of audio file, could not conform";
    return false;
  }

  // Create resampling context
  SwrContext* resampler = swr_alloc_set_opts(nullptr,
                                             p.channel_layout(),
                                             FFmpegCommon::GetFFmpegSampleFormat(p.format()),
                                             p.sample_rate(),
                                             channel_layout,
                                             static_cast<AVSampleFormat>(index_instance.stream()->codecpar->format),
                                             index_instance.stream()->codecpar->sample_rate,
                                             0,
                                             nullptr);

  swr_init(resampler);

  WaveOutput wave_out(conformed_fn, p);

  AVPacket* pkt = av_packet_alloc();
  AVFrame* frame = av_frame_alloc();
  int ret;

  bool success = false;

  if (wave_out.open()) {
    while (true) {
      // Check if we have a `cancelled` ptr and its value
      if (cancelled && *cancelled) {
        break;
      }

      ret = index_instance.GetFrame(pkt, frame);

      if (ret < 0) {

        if (ret == AVERROR_EOF) {
          success = true;
        } else {
          char err_str[50];
          av_strerror(ret, err_str, 50);
          qWarning() << "Failed to conform:" << ret << err_str;
        }
        break;

      }

      // Allocate buffers
      int nb_samples = swr_get_out_samples(resampler, frame->nb_samples);
      char* data = new char[p.samples_to_bytes(nb_samples)];

      // Resample audio to our destination parameters
      nb_samples = swr_convert(resampler,
                               reinterpret_cast<uint8_t**>(&data),
                               nb_samples,
                               const_cast<const uint8_t**>(frame->data),
                               frame->nb_samples);

      if (nb_samples < 0) {
        char err_str[50];
        av_strerror(nb_samples, err_str, 50);
        qWarning() << "libswresample failed with error:" << nb_samples << err_str;
        break;
      }

      // Write packed WAV data to the disk cache
      wave_out.write(data, p.samples_to_bytes(nb_samples));

      // If we allocated an output for the resampler, delete it here
      if (data != reinterpret_cast<char*>(frame->data[0])) {
        delete [] data;
      }

      SignalProcessingProgress(frame->pts);
    }

    wave_out.close();

    if (success) {

      // If our conform succeeded, add it
      audio_stream->append_conformed_version(p);

    } else {

      // Audio index didn't complete, delete it
      QFile(conformed_fn).remove();

    }
  } else {
    qWarning() << "Failed to open WAVE output for indexing";
  }

  swr_free(&resampler);

  av_frame_free(&frame);
  av_packet_free(&pkt);

  return success;
}

PixelFormat::Format FFmpegDecoder::GetNativePixelFormat(AVPixelFormat pix_fmt)
{
  switch (pix_fmt) {
  case AV_PIX_FMT_RGB24:
    return PixelFormat::PIX_FMT_RGB8;
  case AV_PIX_FMT_RGBA:
    return PixelFormat::PIX_FMT_RGBA8;
  case AV_PIX_FMT_RGB48:
    return PixelFormat::PIX_FMT_RGB16U;
  case AV_PIX_FMT_RGBA64:
    return PixelFormat::PIX_FMT_RGBA16U;
  default:
    return PixelFormat::PIX_FMT_INVALID;
  }
}

uint64_t FFmpegDecoder::ValidateChannelLayout(AVStream* stream)
{
  if (stream->codecpar->channel_layout) {
    return stream->codecpar->channel_layout;
  }

  return av_get_default_channel_layout(stream->codecpar->channels);
}

bool FFmpegDecoder::StreamUsesMultipleInstances(StreamPtr stream)
{
  return stream->type() == Stream::kVideo
      && std::static_pointer_cast<VideoStream>(stream)->video_type() != VideoStream::kVideoTypeStill;
}

FramePtr FFmpegDecoder::BuffersToNativeFrame(int divider, int width, int height, const rational& ts, uint8_t** input_data, int* input_linesize)
{
  if (divider != scale_divider_) {
    FreeScaler();
    InitScaler(divider);
  }

  // Create frame to return
  FramePtr copy = Frame::Create();
  copy->set_video_params(VideoParams(width,
                                     height,
                                     native_pix_fmt_,
                                     std::static_pointer_cast<VideoStream>(stream())->pixel_aspect_ratio(),
                                     std::static_pointer_cast<VideoStream>(stream())->interlacing(),
                                     divider));
  copy->set_timestamp(ts);
  copy->allocate();

  // Convert frame to RGB/A for the rest of the pipeline
  uint8_t* output_data = reinterpret_cast<uint8_t*>(copy->data());
  int output_linesize = copy->linesize_bytes();

  sws_scale(scale_ctx_,
            input_data,
            input_linesize,
            0,
            VideoParams::GetScaledDimension(height, divider),
            &output_data,
            &output_linesize);

  return copy;
}

int FFmpegDecoderInstance::GetFrame(AVPacket *pkt, AVFrame *frame)
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

QMutex *FFmpegDecoderInstance::cache_lock()
{
  return &cache_lock_;
}

QWaitCondition *FFmpegDecoderInstance::cache_wait_cond()
{
  return &cache_wait_cond_;
}

bool FFmpegDecoderInstance::IsWorking()
{
  QMutexLocker locker(&is_working_mutex_);
  return is_working_;
}

void FFmpegDecoderInstance::SetWorking(bool working)
{
  QMutexLocker locker(&is_working_mutex_);
  is_working_ = working;
}

void FFmpegDecoderInstance::Seek(int64_t timestamp)
{
  avcodec_flush_buffers(codec_ctx_);
  av_seek_frame(fmt_ctx_, avstream_->index, timestamp, AVSEEK_FLAG_BACKWARD);
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

void FFmpegDecoderInstance::ClearFrameCache()
{
  cached_frames_.clear();
  cache_at_eof_ = false;
  cache_at_zero_ = false;
}

FFmpegFramePool::ElementPtr FFmpegDecoderInstance::RetrieveFrame(const int64_t& target_ts, int divider, bool cache_is_locked)
{
  if (!cache_is_locked) {
    cache_lock_.lock();
  }

  if (scale_divider_ != divider) {
    FreeScaler();
    InitScaler(divider);
  }

  int64_t seek_ts = target_ts;
  bool still_seeking = false;

  // CacheCouldContainTime uses cache_target_time_, so we'll temporarily set it to the last frame's TS
  if (!cached_frames_.isEmpty()) {
    cache_target_time_ = cached_frames_.last()->timestamp();
  }

  // If the frame wasn't in the frame cache, see if this frame cache is too old to use
  if (!CacheCouldContainTime(target_ts)) {
    ClearFrameCache();

    Seek(seek_ts);
    if (seek_ts == 0) {
      cache_at_zero_ = true;
    }

    still_seeking = true;
  }

  cache_target_time_ = target_ts;

  int ret;
  AVPacket* pkt = av_packet_alloc();
  FFmpegFramePool::ElementPtr return_frame = nullptr;

  // Allocate a new frame
  AVFrameWrapper working_frame;

  bool unlocked = false;

  while (true) {

    // Pull from the decoder
    ret = GetFrame(pkt, working_frame.frame());

    // Handle any errors that aren't EOF (EOF is handled later on)
    if (ret < 0 && ret != AVERROR_EOF) {
      cache_lock_.unlock();
      qCritical() << "Failed to retrieve frame:" << ret;
      break;
    }

    if (still_seeking) {
      // Handle a failure to seek (occurs on some media)
      // We'll only be here if the frame cache was emptied earlier
      if (!cache_at_zero_ && (ret == AVERROR_EOF || working_frame.frame()->pts > target_ts)) {

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

    if (cache_is_locked) {
      cache_is_locked = false;
    } else if (unlocked) {
      cache_lock_.lock();
    }

    if (ret == AVERROR_EOF) {

      // Handle an "expected" EOF by using the last frame of our cache
      cache_at_eof_ = true;

      if (cached_frames_.isEmpty()) {
        qCritical() << "Unexpected codec EOF - unable to retrieve frame";
      } else {
        return_frame = cached_frames_.last();
      }

      cache_wait_cond_.wakeAll();
      cache_lock_.unlock();
      break;

    } else {

      // Whatever it is, keep this frame in memory for the time being just in case
      if (!frame_pool_) {
        qCritical() << "Cannot retrieve video without a valid frame pool";
        cache_lock_.unlock();
        break;
      }

      // Cut down to thread count - 1 before we acquire a new frame
      TruncateCacheRangeToFrames(QThread::idealThreadCount() -1);

      FFmpegFramePool::ElementPtr cached = frame_pool_->Get();

      if (!cached) {
        qCritical() << "Frame pool failed to return a valid frame - out of memory?";
        cache_lock_.unlock();
        break;
      }

      {
        uint8_t* scale_data[4];
        int scale_linesize[4];

        av_image_fill_arrays(scale_data,
                             scale_linesize,
                             cached->data(),
                             static_cast<AVPixelFormat>(working_frame.frame()->format),
                             VideoParams::GetScaledDimension(working_frame.frame()->width, divider),
                             VideoParams::GetScaledDimension(working_frame.frame()->height, divider),
                             1);

        sws_scale(scale_ctx_,
                  working_frame.frame()->data,
                  working_frame.frame()->linesize,
                  0,
                  working_frame.frame()->height,
                  scale_data,
                  scale_linesize);
      }

      // Set timestamp so this frame can be identified later
      cached->set_timestamp(working_frame.frame()->pts);

      // Store frame before just in case
      FFmpegFramePool::ElementPtr previous;
      if (cached_frames_.isEmpty()) {
        previous = nullptr;
      } else {
        previous = cached_frames_.last();
      }

      // Append this frame and signal to other threads that a new frame has arrived
      cached_frames_.append(cached);

      cache_wait_cond_.wakeAll();
      cache_lock_.unlock();
      unlocked = true;

      // If this is a valid frame, see if this or the frame before it are the one we need
      if (cached->timestamp() == target_ts) {
        return_frame = cached;
        break;
      } else if (cached->timestamp() > target_ts) {
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

  av_packet_free(&pkt);

  return return_frame;
}

void FFmpegDecoder::ClearResources()
{
  FreeScaler();

  open_ = false;
}

void FFmpegDecoder::InitScaler(int divider)
{
  VideoStream* vs = static_cast<VideoStream*>(stream().get());

  int scaled_width = VideoParams::GetScaledDimension(vs->width(), divider);
  int scaled_height = VideoParams::GetScaledDimension(vs->height(), divider);

  scale_ctx_ = sws_getContext(scaled_width,
                              scaled_height,
                              src_pix_fmt_,
                              scaled_width,
                              scaled_height,
                              ideal_pix_fmt_,
                              SWS_FAST_BILINEAR,
                              nullptr,
                              nullptr,
                              nullptr);

  if (scale_ctx_) {
    scale_divider_ = divider;
  } else {
    scale_divider_ = 0;
  }
}

void FFmpegDecoder::FreeScaler()
{
  if (scale_ctx_) {
    sws_freeContext(scale_ctx_);
    scale_ctx_ = nullptr;

    scale_divider_ = 0;
  }
}

void FFmpegDecoderInstance::InitScaler(int divider)
{
  int scaled_width = VideoParams::GetScaledDimension(avstream_->codecpar->width, divider);
  int scaled_height = VideoParams::GetScaledDimension(avstream_->codecpar->height, divider);

  scale_ctx_ = sws_getContext(avstream_->codecpar->width,
                              avstream_->codecpar->height,
                              static_cast<AVPixelFormat>(avstream_->codecpar->format),
                              scaled_width,
                              scaled_height,
                              static_cast<AVPixelFormat>(avstream_->codecpar->format),
                              SWS_FAST_BILINEAR,
                              nullptr,
                              nullptr,
                              nullptr);

  if (scale_ctx_) {
    scale_divider_ = divider;
  } else {
    scale_divider_ = 0;
  }
}

void FFmpegDecoderInstance::FreeScaler()
{
  if (scale_ctx_) {
    sws_freeContext(scale_ctx_);
    scale_ctx_ = nullptr;

    scale_divider_ = 0;
  }
}

int64_t FFmpegDecoderInstance::RangeStart() const
{
  if (cached_frames_.isEmpty()) {
    return AV_NOPTS_VALUE;
  }
  return cached_frames_.first()->timestamp();
}

int64_t FFmpegDecoderInstance::RangeEnd() const
{
  if (cached_frames_.isEmpty()) {
    return AV_NOPTS_VALUE;
  }
  return cached_frames_.last()->timestamp();
}

bool FFmpegDecoderInstance::CacheContainsTime(const int64_t &t) const
{
  return !cached_frames_.isEmpty()
      && ((RangeStart() <= t && RangeEnd() >= t)
          || (cache_at_zero_ && t < cached_frames_.first()->timestamp())
          || (cache_at_eof_ && t > cached_frames_.last()->timestamp()));
}

bool FFmpegDecoderInstance::CacheWillContainTime(const int64_t &t) const
{
  return !cached_frames_.isEmpty() && t >= cached_frames_.first()->timestamp() && t <= cache_target_time_;
}

bool FFmpegDecoderInstance::CacheCouldContainTime(const int64_t &t) const
{
  return !cached_frames_.isEmpty() && t >= cached_frames_.first()->timestamp() && t <= (cache_target_time_ + 2*second_ts_);
}

bool FFmpegDecoderInstance::CacheIsEmpty() const
{
  return cached_frames_.isEmpty();
}

FFmpegFramePool::ElementPtr FFmpegDecoderInstance::GetFrameFromCache(const int64_t &t) const
{
  if (t < cached_frames_.first()->timestamp()) {

    if (cache_at_zero_) {
      cached_frames_.first()->access();
      return cached_frames_.first();
    }

  } else if (t > cached_frames_.last()->timestamp()) {

    if (cache_at_eof_) {
      cached_frames_.last()->access();
      return cached_frames_.last();
    }

  } else {

    // We already have this frame in the cache, find it
    for (int i=0;i<cached_frames_.size();i++) {
      FFmpegFramePool::ElementPtr this_frame = cached_frames_.at(i);

      if (this_frame->timestamp() == t // Test for an exact match
          || (i < cached_frames_.size() - 1 && cached_frames_.at(i+1)->timestamp() > t)) { // Or for this frame to be the "closest"

        this_frame->access();
        return this_frame;

      }
    }
  }

  return nullptr;
}

void FFmpegDecoderInstance::RemoveFramesBefore(const qint64 &t)
{
  // We keep one frame in memory as an identifier for what pts the decoder is up to
  int min_frames = (MemoryPoolLimitReached() && !IsWorking()) ? 0 : 1;

  while (cached_frames_.size() > min_frames && cached_frames_.first()->last_accessed() < t) {
    RemoveFirstFrame();
  }
}

int FFmpegDecoderInstance::TruncateCacheRangeToTime(const qint64 &t)
{
  int counter = 0;

  // We keep one frame in memory as an identifier for what pts the decoder is up to
  while (cached_frames_.size() > 1 && (RangeEnd() - RangeStart()) > t) {
    RemoveFirstFrame();
    counter++;
  }

  return counter;
}

int FFmpegDecoderInstance::TruncateCacheRangeToFrames(int nb_frames)
{
  int counter = 0;

  // We keep one frame in memory as an identifier for what pts the decoder is up to
  while (cached_frames_.size() > nb_frames) {
    RemoveFirstFrame();
    counter++;
  }

  return counter;
}

void FFmpegDecoderInstance::RemoveFirstFrame()
{
  cached_frames_.removeFirst();
  cache_at_zero_ = false;
}

FFmpegDecoderInstance::FFmpegDecoderInstance(const char *filename, int stream_index) :
  fmt_ctx_(nullptr),
  codec_ctx_(nullptr),
  opts_(nullptr),
  scale_ctx_(nullptr),
  scale_divider_(0),
  frame_pool_(nullptr),
  is_working_(false),
  cache_at_zero_(false),
  cache_at_eof_(false),
  clear_timer_(nullptr)
{
  // Open file in a format context
  int error_code = avformat_open_input(&fmt_ctx_, filename, nullptr, nullptr);

  // Handle format context error
  if (error_code != 0) {
    qCritical() << "Failed to open input:" << filename << error_code;
    ClearResources();
    return;
  }

  // Get stream information from format
  error_code = avformat_find_stream_info(fmt_ctx_, nullptr);

  // Handle get stream information error
  if (error_code < 0) {
    qCritical() << "Failed to find stream info:" << error_code;
    ClearResources();
    return;
  }

  // Get reference to correct AVStream
  avstream_ = fmt_ctx_->streams[stream_index];

  // Find decoder
  AVCodec* codec = avcodec_find_decoder(avstream_->codecpar->codec_id);

  // Handle failure to find decoder
  if (codec == nullptr) {
    qCritical() << "Failed to find appropriate decoder for this codec:" << filename << stream_index << avstream_->codecpar->codec_id;
    ClearResources();
    return;
  }

  // Allocate context for the decoder
  codec_ctx_ = avcodec_alloc_context3(codec);
  if (codec_ctx_ == nullptr) {
    qCritical() << "Failed to allocate codec context";
    ClearResources();
    return;
  }

  // Copy parameters from the AVStream to the AVCodecContext
  error_code = avcodec_parameters_to_context(codec_ctx_, avstream_->codecpar);

  // Handle failure to copy parameters
  if (error_code < 0) {
    qCritical() << "Failed to copy parameters from AVStream to AVCodecContext";
    ClearResources();
    return;
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
    char buf[50];
    av_strerror(error_code, buf, 50);
    qCritical() << "Failed to open codec" << codec->id << error_code << buf;
    ClearResources();
    return;
  }

  // Create frame pool
  if (avstream_->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
    // Start clear timer
    clear_timer_ = new QTimer();
    clear_timer_->setInterval(kMaxFrameLife);
    clear_timer_->moveToThread(qApp->thread());
    connect(clear_timer_, &QTimer::timeout, this, &FFmpegDecoderInstance::ClearTimerEvent, Qt::DirectConnection);
    QMetaObject::invokeMethod(clear_timer_, "start", Qt::QueuedConnection);
  }

  // Store one second in the source's timebase
  second_ts_ = qRound64(av_q2d(av_inv_q(avstream_->time_base)));
}

FFmpegDecoderInstance::~FFmpegDecoderInstance()
{
  ClearResources();
}

bool FFmpegDecoderInstance::IsValid() const
{
  return codec_ctx_;
}

void FFmpegDecoderInstance::SetFramePool(FFmpegFramePool *frame_pool)
{
  frame_pool_ = frame_pool;
}

void FFmpegDecoderInstance::ClearResources()
{
  ClearFrameCache();

  // Stop timer
  if (clear_timer_) {
    QMetaObject::invokeMethod(clear_timer_, "stop", Qt::QueuedConnection);
    clear_timer_->deleteLater();
    clear_timer_ = nullptr;
  }

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

  FreeScaler();
}

void FFmpegDecoderInstance::ClearTimerEvent()
{
  cache_lock()->lock();
  RemoveFramesBefore(QDateTime::currentMSecsSinceEpoch() - kMaxFrameLife);
  cache_lock()->unlock();
}

uint qHash(const FFmpegDecoder::FFmpegFramePoolKey &r)
{
  return ::qHash(r.width) ^ ::qHash(r.height) ^ ::qHash(r.format);
}

OLIVE_NAMESPACE_EXIT
