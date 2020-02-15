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
#include "common/timecodefunctions.h"
#include "ffmpegcommon.h"
#include "render/diskmanager.h"
#include "render/pixelservice.h"

FFmpegDecoder::FFmpegDecoder() :
  fmt_ctx_(nullptr),
  codec_ctx_(nullptr),
  scale_ctx_(nullptr),
  pkt_(nullptr),
  frame_(nullptr),
  opts_(nullptr),
  multithreading_(false)
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
  error_code = av_dict_set(&opts_, "threads", multithreading_ ? "auto" : "1", 0);

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
  }

  pkt_ = av_packet_alloc();
  if (!pkt_) {
    Error(QStringLiteral("Failed to allocate AVPacket"));
    return false;
  }

  frame_ = av_frame_alloc();
  if (!frame_) {
    Error(QStringLiteral("Failed to allocate AVFrame"));
    return false;
  }

  // All allocation succeeded so we set the state to open
  open_ = true;

  return true;
}

FramePtr FFmpegDecoder::RetrieveVideo(const rational &timecode)
{
  if (!open_ && !Open()) {
    return nullptr;
  }

  if (avstream_->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
    return nullptr;
  }

  // Convert timecode to AVStream timebase
  int64_t target_ts = GetTimestampFromTime(timecode);

  if (target_ts < 0) {
    Error(QStringLiteral("Index failed to produce a valid timestamp"));
    return nullptr;
  }

  // Allocate frame that we'll return
  FramePtr frame_container = Frame::Create();
  frame_container->set_width(avstream_->codecpar->width);
  frame_container->set_height(avstream_->codecpar->height);
  frame_container->set_format(native_pix_fmt_);
  frame_container->set_timestamp(Timecode::timestamp_to_time(target_ts, avstream_->time_base));
  frame_container->set_sample_aspect_ratio(av_guess_sample_aspect_ratio(fmt_ctx_, avstream_, nullptr));
  frame_container->allocate();

  bool got_frame = (frame_->pts == target_ts);

  QByteArray frame_loader;
  uint8_t* input_data[4];
  int input_linesize[4];

  // If we already have the frame, we'll need to set the pointers to it
  for (int i=0;i<4;i++) {
    input_data[i] = frame_->data[i];
    input_linesize[i] = frame_->linesize[i];
  }

  // See if we stored this frame in the disk cache
  if (!got_frame) {
    QFile compressed_frame(GetIndexFilename().append(QString::number(target_ts)));
    if (compressed_frame.exists()
        && compressed_frame.size() > 0
        && compressed_frame.open(QFile::ReadOnly)) {
      DiskManager::instance()->Accessed(compressed_frame.fileName());

      // Read data
      frame_loader = qUncompress(compressed_frame.readAll());
      //frame_loader = compressed_frame.readAll();

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

  // If we have no disk cache, we'll need to find this frame ourselves
  if (!got_frame) {
    int64_t second_ts = qRound64(av_q2d(av_inv_q(avstream_->time_base)));

    if (frame_->pts < target_ts - 2*second_ts || frame_->pts > target_ts) {
      Seek(target_ts);
    }

    int64_t seek_ts = target_ts;

    int ret;

    while (true) {
      ret = GetFrame(pkt_, frame_);

      if (ret < 0) {
        FFmpegError(ret);
        break;
      }

      if (frame_->pts > target_ts) {
        // Seek failed, try again
        seek_ts -= second_ts;
        Seek(seek_ts);
        continue;
      }

      if (frame_->pts == target_ts) {
        // We found the frame we want
        got_frame = true;

        // Set data arrays to the frame's data
        for (int i=0;i<4;i++) {
          input_data[i] = frame_->data[i];
          input_linesize[i] = frame_->linesize[i];
        }

        CacheFrameToDisk(frame_);
        break;
      }
    }
  }

  // If we're here and got the frame, we'll convert it and return it
  if (got_frame) {
    // Convert frame to RGBA for the rest of the pipeline
    uint8_t* output_data = reinterpret_cast<uint8_t*>(frame_container->data());
    int output_linesize = frame_container->width() * kRGBAChannels * PixelService::BytesPerChannel(native_pix_fmt_);

    sws_scale(scale_ctx_,
              input_data,
              input_linesize,
              0,
              avstream_->codecpar->height,
              &output_data,
              &output_linesize);

    return frame_container;
  }

  return nullptr;
}

FramePtr FFmpegDecoder::RetrieveAudio(const rational &timecode, const rational &length, const AudioRenderingParams &params)
{
  if (!open_ && !Open()) {
    return nullptr;
  }

  if (avstream_->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
    return nullptr;
  }

  Index();

  Conform(params);

  WaveInput input(GetConformedFilename(params));

  // FIXME: No handling if input failed to open/is corrupt
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
  if (opts_) {
    av_dict_free(&opts_);
    opts_ = nullptr;
  }

  if (frame_) {
    av_frame_free(&frame_);
    frame_ = nullptr;
  }

  if (pkt_) {
    av_packet_free(&pkt_);
    pkt_ = nullptr;
  }

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

QString FFmpegDecoder::id()
{
  return "ffmpeg";
}

int64_t FFmpegDecoder::GetTimestampFromTime(const rational &time)
{
  if (!open_ && !Open()) {
    return -1;
  }

  // Get rough approximation of what the timestamp would be in this timebase
  int64_t target_ts = Timecode::time_to_timestamp(time, avstream_->time_base);

  // Adjust target by stream's start time
  target_ts += avstream_->start_time;

  // Find closest actual timebase in the file
  target_ts = GetClosestTimestampInIndex(target_ts);

  return target_ts;
}

void FFmpegDecoder::Conform(const AudioRenderingParams &params)
{
  if (avstream_->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
    // Nothing to be done
    return;
  }

  Index();

  // Get indexed WAV file
  WaveInput input(GetIndexFilename());

  // FIXME: No handling if input failed to open/is corrupt
  if (input.open()) {
    // If the parameters are equal, nothing to be done
    // FIXME: Technically we only need to conform if the SAMPLE RATE is not equal. Format and channel layout conversion
    //        could be done on the fly so we could perhaps conform less often at some point.
    if (input.params() == params) {
      input.close();
      return;
    }

    // Otherwise, let's start converting the format

    // Generate destination filename for this conversion to see if it exists
    QString conformed_fn = GetConformedFilename(params);

    if (QFileInfo::exists(conformed_fn)) {
      // We must have already conformed this format
      input.close();
      return;
    }

    // Set up resampler
    SwrContext* resampler = swr_alloc_set_opts(nullptr,
                                               static_cast<int64_t>(params.channel_layout()),
                                               FFmpegCommon::GetFFmpegSampleFormat(params.format()),
                                               params.sample_rate(),
                                               static_cast<int64_t>(input.params().channel_layout()),
                                               FFmpegCommon::GetFFmpegSampleFormat(input.params().format()),
                                               input.params().sample_rate(),
                                               0,
                                               nullptr);

    swr_init(resampler);

    WaveOutput conformed_output(conformed_fn, params);
    if (!conformed_output.open()) {
      qWarning() << "Failed to open conformed output:" << conformed_fn;
      input.close();
      return;
    }

    // Convert one second of audio at a time
    int input_buffer_sz = input.params().time_to_bytes(1);

    while (!input.at_end()) {
      // Read up to one second of audio from WAV file
      QByteArray read_samples = input.read(input_buffer_sz);

      // Determine how many samples this is
      int in_sample_count = input.params().bytes_to_samples(read_samples.size());

      ConformInternal(resampler, &conformed_output, read_samples.data(), in_sample_count);
    }

    // Flush resampler
    ConformInternal(resampler, &conformed_output, nullptr, 0);

    // Clean up
    swr_free(&resampler);
    conformed_output.close();
    input.close();
  } else {
    qWarning() << "Failed to conform file:" << stream()->footage()->filename();
  }
}

bool FFmpegDecoder::SupportsVideo()
{
  return true;
}

bool FFmpegDecoder::SupportsAudio()
{
  return true;
}

void FFmpegDecoder::SetMultithreading(bool e)
{
  multithreading_ = e;
}

void FFmpegDecoder::ConformInternal(SwrContext* resampler, WaveOutput* output, const char* in_data, int in_sample_count)
{
  // Determine how many samples the output will be
  int out_sample_count = swr_get_out_samples(resampler, in_sample_count);

  // Allocate array for the amount of samples we'll need
  QByteArray out_samples;
  out_samples.resize(output->params().samples_to_bytes(out_sample_count));

  char* out_data = out_samples.data();

  // Convert samples
  int convert_count = swr_convert(resampler,
                                  reinterpret_cast<uint8_t**>(&out_data),
                                  out_sample_count,
                                  reinterpret_cast<const uint8_t**>(&in_data),
                                  in_sample_count);

  if (convert_count != out_sample_count) {
    out_samples.resize(output->params().samples_to_bytes(convert_count));
  }

  output->write(out_samples);
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

  bool need_manual_duration = false;

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
        need_manual_duration = true;
      }

      f->add_stream(str);
    }

    // As long as we can open the container and retrieve information, this was a successful probe
    result = true;
  }

  // Free all memory
  Close();

  // If the metadata did not contain a duration, we'll need to loop through the file to retrieve it
  if (need_manual_duration) {
    // Index the first stream to retrieve the duration

    set_stream(f->stream(0));

    Open();

    // Use index to find duration
    Index();

    // Use last frame index as the duration
    // FIXME: Does this skip the last frame?
    int64_t duration = std::static_pointer_cast<VideoStream>(stream())->last_frame_index_timestamp();

    f->stream(0)->set_duration(duration);

    // Assume all durations are the same and set for each
    for (int i=1;i<f->stream_count();i++) {
      int64_t new_dur = av_rescale_q(duration,
                                     f->stream(0)->timebase().toAVRational(),
                                     f->stream(i)->timebase().toAVRational());

      f->stream(i)->set_duration(new_dur);
    }

    Close();
  }

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

  Close();
}

void FFmpegDecoder::Index()
{
  if (!open_) {
    qWarning() << "Indexing function tried to run while decoder was closed";
    return;
  }

  QMutexLocker locker(stream()->index_process_lock());

  if (stream()->type() == Stream::kVideo) {

    ValidateVideoIndex();

  } else if (stream()->type() == Stream::kAudio) {
    if (!QFileInfo::exists(GetIndexFilename())) {
      UnconditionalAudioIndex(pkt_, frame_);
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

QString FFmpegDecoder::GetConformedFilename(const AudioRenderingParams &params)
{
  QString index_fn = GetIndexFilename();
  WaveInput input(GetIndexFilename());

  // FIXME: No handling if input failed to open/is corrupt
  if (input.open()) {
    // If the parameters are equal, nothing to be done
    AudioRenderingParams index_params = input.params();
    input.close();

    if (index_params == params) {
      // Source file matches perfectly, no conform required
      return index_fn;
    }
  }

  index_fn.append('.');
  index_fn.append(QString::number(params.sample_rate()));
  index_fn.append('.');
  index_fn.append(QString::number(params.format()));
  index_fn.append('.');
  index_fn.append(QString::number(params.channel_layout()));

  return index_fn;
}

void FFmpegDecoder::UnconditionalAudioIndex(AVPacket *pkt, AVFrame *frame)
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

  WaveOutput wave_out(GetIndexFilename(),
                      AudioRenderingParams(avstream_->codecpar->sample_rate,
                                           channel_layout,
                                           FFmpegCommon::GetNativeSampleFormat(dst_sample_fmt)));

  int ret;

  if (wave_out.open()) {
    while (true) {
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

        // If we allocated an output for the resampler, delete it here
        if (data_frame != frame) {
          av_frame_free(&data_frame);
        }
      }
    }

    wave_out.close();
  } else {
    qWarning() << "Failed to open WAVE output for indexing";
  }

  if (resampler != nullptr) {
    swr_free(&resampler);
  }

  Seek(0);
}

void FFmpegDecoder::UnconditionalVideoIndex(AVPacket* pkt, AVFrame* frame)
{
  VideoStreamPtr video_stream = std::static_pointer_cast<VideoStream>(stream());

  Seek(0);

  // This should be unnecessary, but just in case...
  video_stream->clear_frame_index();

  // Iterate through every single frame and get each timestamp
  // NOTE: Expects no frames to have been read so far

  int ret;

  while (true) {
    ret = GetFrame(pkt, frame);

    if (ret >= 0) {
      //CacheFrameToDisk(frame);

      video_stream->append_frame_index(frame->pts);
    } else {
      // Assume we've reached the end of the file
      break;
    }
  }

  video_stream->append_frame_index(VideoStream::kEndTimestamp);

  // Save index to file
  if (!video_stream->save_frame_index(GetIndexFilename())) {
    qWarning() << QStringLiteral("Failed to save index for %1").arg(stream()->footage()->filename());
  }

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
      // Handle other error
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

int64_t FFmpegDecoder::GetClosestTimestampInIndex(const int64_t &ts)
{
  VideoStreamPtr video_stream = std::static_pointer_cast<VideoStream>(stream());

  bool index_is_being_created = false;

  // Check if an index is being created right now
  if (video_stream->index_process_lock()->tryLock()) {

    // If not, check if the frame index has been populated
    if (!video_stream->is_frame_index_ready()) {
      // If not, make a frame index
      ValidateVideoIndex();
    }

    video_stream->index_process_lock()->unlock();

  } else {
    index_is_being_created = true;
  }

  int64_t closest_ts = -1;

  do {
    if (index_is_being_created && video_stream->index_process_lock()->tryLock()) {
      index_is_being_created = false;
      video_stream->index_process_lock()->unlock();
    }

    // FIXME: Wait for update from index
    //WaitForUpdate();

    closest_ts = video_stream->get_closest_timestamp_in_frame_index(ts);

  } while (closest_ts < 0 && index_is_being_created);

  return closest_ts;
}

void FFmpegDecoder::ValidateVideoIndex()
{
  VideoStreamPtr video_stream = std::static_pointer_cast<VideoStream>(stream());

  if (!video_stream->is_frame_index_ready()) {
    video_stream->load_frame_index(GetIndexFilename());
  }

  if (!video_stream->is_frame_index_ready()) {
    // Reset state
    Seek(0);

    UnconditionalVideoIndex(pkt_, frame_);

    Seek(0);
  }
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
