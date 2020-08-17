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

#include "ffmpegencoder.h"

extern "C" {
#include <libavutil/pixdesc.h>
}

#include <QFile>

#include "ffmpegcommon.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

FFmpegEncoder::FFmpegEncoder(const EncodingParams &params) :
  Encoder(params),
  fmt_ctx_(nullptr),
  video_stream_(nullptr),
  video_codec_ctx_(nullptr),
  video_scale_ctx_(nullptr),
  audio_stream_(nullptr),
  audio_codec_ctx_(nullptr),
  audio_resample_ctx_(nullptr),
  open_(false)
{
}

bool FFmpegEncoder::Open()
{
  if (open_) {
    return true;
  }

  int error_code;

  // Convert QString to C string that FFmpeg expects
  QByteArray filename_bytes = params().filename().toUtf8();
  const char* filename_c_str = filename_bytes.constData();

  // Create output format context
  error_code = avformat_alloc_output_context2(&fmt_ctx_, nullptr, nullptr, filename_c_str);

  // Check error code
  if (error_code < 0) {
    FFmpegError("Failed to allocate output context", error_code);
    return false;
  }

  // Initialize a video stream if it's enabled
  if (params().video_enabled()) {
    if (!InitializeStream(AVMEDIA_TYPE_VIDEO, &video_stream_, &video_codec_ctx_, params().video_codec())) {
      return false;
    }

    // This is the format we will expect frames received in Write() to be in
    PixelFormat::Format native_pixel_fmt = params().video_params().format();

    // This is the format we will need to convert the frame to for swscale to understand it
    video_conversion_fmt_ = FFmpegCommon::GetCompatiblePixelFormat(native_pixel_fmt);

    // This is the equivalent pixel format above as an AVPixelFormat that swscale can understand
    AVPixelFormat src_pix_fmt = FFmpegCommon::GetFFmpegPixelFormat(video_conversion_fmt_);

    // This is the pixel format the encoder wants to encode to
    AVPixelFormat encoder_pix_fmt = video_codec_ctx_->pix_fmt;

    // Set up a scaling context - if the native pixel format is not equal to the encoder's, we'll need to convert it
    // before encoding. Even if we don't, this may be useful for converting between linesizes, etc.
    video_scale_ctx_ = sws_getContext(params().video_params().width(),
                                      params().video_params().height(),
                                      src_pix_fmt,
                                      params().video_params().width(),
                                      params().video_params().height(),
                                      encoder_pix_fmt,
                                      0,
                                      nullptr,
                                      nullptr,
                                      nullptr);
  }

  // Initialize an audio stream if it's enabled
  if (params().audio_enabled()
      && !InitializeStream(AVMEDIA_TYPE_AUDIO, &audio_stream_, &audio_codec_ctx_, params().audio_codec())) {
    return false;
  }

  av_dump_format(fmt_ctx_, 0, filename_c_str, 1);

  // Open output file for writing
  error_code = avio_open(&fmt_ctx_->pb, filename_c_str, AVIO_FLAG_WRITE);
  if (error_code < 0) {
    FFmpegError("Failed to open IO context", error_code);
    return false;
  }

  // Write header
  error_code = avformat_write_header(fmt_ctx_, nullptr);
  if (error_code < 0) {
    FFmpegError("Failed to write format header", error_code);
    return false;
  }

  open_ = true;
  return true;
}

bool FFmpegEncoder::WriteFrame(FramePtr frame, rational time)
{
  bool success = false;

  AVFrame* encoded_frame = av_frame_alloc();

  int error_code;
  const char* input_data;
  int input_linesize;

  // Frame must be video
  encoded_frame->width = frame->width();
  encoded_frame->height = frame->height();
  encoded_frame->format = video_codec_ctx_->pix_fmt;

  // Set interlacing
  if (frame->video_params().interlacing() != VideoParams::kInterlaceNone) {
    encoded_frame->interlaced_frame = 1;

    if (frame->video_params().interlacing() == VideoParams::kInterlacedTopFirst) {
      encoded_frame->top_field_first = 1;
    } else {
      encoded_frame->top_field_first = 0;
    }
  }

  error_code = av_frame_get_buffer(encoded_frame, 0);
  if (error_code < 0) {
    FFmpegError("Failed to create AVFrame buffer", error_code);
    goto fail;
  }

  // We may need to convert this frame to a frame that swscale will understand
  if (frame->format() != video_conversion_fmt_) {
    frame = PixelFormat::ConvertPixelFormat(frame, video_conversion_fmt_);
  }

  // Use swscale context to convert formats/linesizes
  input_data = frame->const_data();
  input_linesize = frame->linesize_bytes();
  error_code = sws_scale(video_scale_ctx_,
                         reinterpret_cast<const uint8_t**>(&input_data),
                         &input_linesize,
                         0,
                         frame->height(),
                         encoded_frame->data,
                         encoded_frame->linesize);
  if (error_code < 0) {
    FFmpegError("Failed to scale frame", error_code);
    goto fail;
  }

  encoded_frame->pts = qRound64(time.toDouble() / av_q2d(video_codec_ctx_->time_base));

  success = WriteAVFrame(encoded_frame, video_codec_ctx_, video_stream_);

fail:
  av_frame_free(&encoded_frame);

  return success;
}

void FFmpegEncoder::WriteAudio(AudioParams pcm_info, const QString &pcm_filename)
{
  QFile pcm(pcm_filename);
  if (pcm.open(QFile::ReadOnly)) {
    // Divide PCM stream into AVFrames

    // See if the codec defines a number of samples per frame
    int maximum_frame_samples = audio_codec_ctx_->frame_size;
    if (!maximum_frame_samples) {
      // If not, use another frame size
      if (params().video_enabled()) {
        // If we're encoding video, use enough samples to cover roughly one frame of video
        maximum_frame_samples = params().audio_params().time_to_samples(params().video_params().time_base());
      } else {
        // If no video, just use an arbitrary number
        maximum_frame_samples = 256;
      }
    }

    SwrContext* swr_ctx = swr_alloc_set_opts(nullptr,
                                             static_cast<int64_t>(audio_codec_ctx_->channel_layout),
                                             audio_codec_ctx_->sample_fmt,
                                             audio_codec_ctx_->sample_rate,
                                             static_cast<int64_t>(pcm_info.channel_layout()),
                                             FFmpegCommon::GetFFmpegSampleFormat(pcm_info.format()),
                                             pcm_info.sample_rate(),
                                             0,
                                             nullptr);

    swr_init(swr_ctx);

    // Loop through PCM queueing write events
    AVFrame* frame = av_frame_alloc();

    // Set up frame and allocate its buffers
    frame->channel_layout = audio_codec_ctx_->channel_layout;
    frame->nb_samples = maximum_frame_samples;
    frame->format = audio_codec_ctx_->sample_fmt;
    av_frame_get_buffer(frame, 0);

    // Keep track of sample count to use as each frame's timebase
    int sample_counter = 0;

    while (true) {
      // Calculate how many samples should input this frame
      int64_t samples_needed = av_rescale_rnd(maximum_frame_samples + swr_get_delay(swr_ctx, pcm_info.sample_rate()),
                                              audio_codec_ctx_->sample_rate,
                                              pcm_info.sample_rate(),
                                              AV_ROUND_UP);

      // Calculate how many bytes this is
      int max_read = pcm_info.samples_to_bytes(samples_needed);

      // Read bytes from PCM
      QByteArray input_data = pcm.read(max_read);

      // Use swresample to convert the data into the correct format
      const char* input_data_array = input_data.constData();
      int converted = swr_convert(swr_ctx,

                                  // output data
                                  frame->data,

                                  // output sample count (maximum amount of samples in output)
                                  maximum_frame_samples,

                                  // input data
                                  reinterpret_cast<const uint8_t**>(&input_data_array),

                                  // input sample count (maximum amount of samples we read from pcm file)
                                  pcm_info.bytes_to_samples(input_data.size()));

      // Update the frame's number of samples to the amount we actually received
      frame->nb_samples = converted;

      // Update frame timestamp
      frame->pts = sample_counter;

      // Increment timestamp for the next frame by the amount of samples in this one
      sample_counter += converted;

      // Write the frame
      if (!WriteAVFrame(frame, audio_codec_ctx_, audio_stream_)) {
        qCritical() << "Failed to write audio AVFrame";
        break;
      }

      // Break if we've reached the end point
      if (pcm.atEnd()) {
        break;
      }
    }

    av_frame_free(&frame);

    swr_free(&swr_ctx);

    pcm.close();
  }
}

void FFmpegEncoder::Close()
{
  if (open_) {
    // Flush encoders
    FlushEncoders();

    // We've written a header, so we'll write a trailer
    av_write_trailer(fmt_ctx_);
    avio_closep(&fmt_ctx_->pb);

    open_ = false;
  }

  if (video_scale_ctx_) {
    sws_freeContext(video_scale_ctx_);
    video_scale_ctx_ = nullptr;
  }

  if (video_codec_ctx_) {
    avcodec_free_context(&video_codec_ctx_);
    video_codec_ctx_ = nullptr;
  }

  if (audio_codec_ctx_) {
    avcodec_free_context(&audio_codec_ctx_);
    audio_codec_ctx_ = nullptr;
  }

  if (fmt_ctx_) {
    // NOTE: This also frees video_stream_ and audio_stream_
    avformat_free_context(fmt_ctx_);
    fmt_ctx_ = nullptr;
  }
}

void FFmpegEncoder::FFmpegError(const char* context, int error_code)
{
  char err[128];
  av_strerror(error_code, err, 128);

  Error(QStringLiteral("%1 for %2 - %3 %4").arg(context,
                                                params().filename(),
                                                QString::number(error_code),
                                                err));
}

bool FFmpegEncoder::WriteAVFrame(AVFrame *frame, AVCodecContext* codec_ctx, AVStream* stream)
{
  // Send raw frame to the encoder
  int error_code = avcodec_send_frame(codec_ctx, frame);
  if (error_code < 0) {
    FFmpegError("Failed to send frame to encoder", error_code);
    return false;
  }

  bool succeeded = false;

  AVPacket* pkt = av_packet_alloc();

  // Retrieve packets from encoder
  while (error_code >= 0) {
    error_code = avcodec_receive_packet(codec_ctx, pkt);

    // EAGAIN just means the encoder wants another frame before encoding
    if (error_code == AVERROR(EAGAIN)) {
      break;
    } else if (error_code < 0) {
      FFmpegError("Failed to receive packet from decoder", error_code);
      goto fail;
    }

    // Set packet stream index
    pkt->stream_index = stream->index;

    av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);

    // Write packet to file
    av_interleaved_write_frame(fmt_ctx_, pkt);

    // Unref packet in case we're getting another
    av_packet_unref(pkt);
  }

  succeeded = true;

fail:
  av_packet_free(&pkt);

  return succeeded;
}

bool FFmpegEncoder::InitializeStream(AVMediaType type, AVStream** stream_ptr, AVCodecContext** codec_ctx_ptr, const ExportCodec::Codec& codec)
{
  if (type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO) {
    Error(QStringLiteral("Cannot initialize a stream that is not a video or audio type"));
    return false;
  }

  // Retrieve codec
  AVCodecID codec_id = AV_CODEC_ID_NONE;

  switch (codec) {
  case ExportCodec::kCodecDNxHD:
    codec_id = AV_CODEC_ID_DNXHD;
    break;
  case ExportCodec::kCodecAAC:
    codec_id = AV_CODEC_ID_AAC;
    break;
  case ExportCodec::kCodecMP2:
    codec_id = AV_CODEC_ID_MP2;
    break;
  case ExportCodec::kCodecMP3:
    codec_id = AV_CODEC_ID_MP3;
    break;
  case ExportCodec::kCodecH264:
    codec_id = AV_CODEC_ID_H264;
    break;
  case ExportCodec::kCodecH265:
    codec_id = AV_CODEC_ID_HEVC;
    break;
  case ExportCodec::kCodecOpenEXR:
    codec_id = AV_CODEC_ID_EXR;
    break;
  case ExportCodec::kCodecPNG:
    codec_id = AV_CODEC_ID_PNG;
    break;
  case ExportCodec::kCodecTIFF:
    codec_id = AV_CODEC_ID_TIFF;
    break;
  case ExportCodec::kCodecProRes:
    codec_id = AV_CODEC_ID_PRORES;
    break;
  case ExportCodec::kCodecPCM:
    codec_id = AV_CODEC_ID_PCM_S16LE;
    break;
  case ExportCodec::kCodecCount:
    break;
  }

  if (codec_id == AV_CODEC_ID_NONE) {
    Error(QStringLiteral("Unknown internal codec"));
    return false;
  }

  // Find encoder with this name
  AVCodec* encoder = avcodec_find_encoder(codec_id);

  if (!encoder) {
    Error(QStringLiteral("Failed to find codec for %1").arg(codec));
    return false;
  }

  if (encoder->type != type) {
    Error(QStringLiteral("Retrieved unexpected codec type %1 for codec %2").arg(QString::number(encoder->type), codec));
    return false;
  }

  if (!InitializeCodecContext(stream_ptr, codec_ctx_ptr, encoder)) {
    return false;
  }

  // Set codec parameters
  AVCodecContext* codec_ctx = *codec_ctx_ptr;
  AVStream* stream = *stream_ptr;

  if (type == AVMEDIA_TYPE_VIDEO) {
    codec_ctx->width = params().video_params().width();
    codec_ctx->height = params().video_params().height();
    codec_ctx->sample_aspect_ratio = params().video_params().pixel_aspect_ratio().toAVRational();
    codec_ctx->time_base = params().video_params().time_base().toAVRational();
    codec_ctx->pix_fmt = av_get_pix_fmt(params().video_pix_fmt().toUtf8());

    if (params().video_params().interlacing() != VideoParams::kInterlaceNone) {
      // FIXME: I actually don't know what these flags do, the documentation helpfully doesn't
      //        explain them at all. I hope using both of them is the right thing to do.
      codec_ctx->flags |= AV_CODEC_FLAG_INTERLACED_DCT | AV_CODEC_FLAG_INTERLACED_ME;


      if (params().video_params().interlacing() == VideoParams::kInterlacedTopFirst) {
        codec_ctx->field_order = AV_FIELD_TT;
      } else {
        codec_ctx->field_order = AV_FIELD_BB;

        if (codec_id == AV_CODEC_ID_H264) {
          // For some reason, FFmpeg doesn't set libx264's bff flag so we have to do it ourselves
          av_opt_set(video_codec_ctx_->priv_data, "x264opts", "bff=1", AV_OPT_SEARCH_CHILDREN);
        }
      }
    }

    // Set custom options
    {
      QHash<QString, QString>::const_iterator i;

      for (i=params().video_opts().begin();i!=params().video_opts().end();i++) {
        av_opt_set(video_codec_ctx_->priv_data, i.key().toUtf8(), i.value().toUtf8(), AV_OPT_SEARCH_CHILDREN);
      }

      if (params().video_bit_rate() > 0) {
        video_codec_ctx_->bit_rate = params().video_bit_rate();
      }

      if (params().video_max_bit_rate() > 0) {
        video_codec_ctx_->rc_max_rate = params().video_max_bit_rate();
      }

      if (params().video_buffer_size() > 0) {
        video_codec_ctx_->rc_buffer_size = static_cast<int>(params().video_buffer_size());
      }
    }

  } else {
    codec_ctx->sample_rate = params().audio_params().sample_rate();
    codec_ctx->channel_layout = params().audio_params().channel_layout();
    codec_ctx->channels = av_get_channel_layout_nb_channels(codec_ctx->channel_layout);
    codec_ctx->sample_fmt = encoder->sample_fmts[0];
    codec_ctx->time_base = {1, codec_ctx->sample_rate};
  }

  if (!SetupCodecContext(stream, codec_ctx, encoder)) {
    return false;
  }

  return true;
}

bool FFmpegEncoder::InitializeCodecContext(AVStream **stream, AVCodecContext **codec_ctx, AVCodec* codec)
{
  *stream = avformat_new_stream(fmt_ctx_, nullptr);
  if (!(*stream)) {
    Error(QStringLiteral("Failed to allocate AVStream"));
    return false;
  }

  // Allocate a codec context
  *codec_ctx = avcodec_alloc_context3(codec);
  if (!(*codec_ctx)) {
    Error(QStringLiteral("Failed to allocate AVCodecContext"));
    return false;
  }

  return true;
}

bool FFmpegEncoder::SetupCodecContext(AVStream* stream, AVCodecContext* codec_ctx, AVCodec* codec)
{
  int error_code;

  if (fmt_ctx_->oformat->flags & AVFMT_GLOBALHEADER) {
    codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  AVDictionary* codec_opts = nullptr;

  // Set thread count
  if (params().video_threads() == 0) {
    av_dict_set(&codec_opts, "threads", "auto", 0);
  } else {
    QString thread_val = QString::number(params().video_threads());
    av_dict_set(&codec_opts, "threads", thread_val.toUtf8(), 0);
  }

  // Try to open encoder
  error_code = avcodec_open2(codec_ctx, codec, &codec_opts);
  if (error_code < 0) {
    FFmpegError("Failed to open encoder", error_code);
    return false;
  }

  // Copy context settings to codecpar object
  error_code = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
  if (error_code < 0) {
    FFmpegError("Failed to copy codec parameters to stream", error_code);
    return false;
  }

  return true;
}

void FFmpegEncoder::FlushEncoders()
{
  if (video_codec_ctx_) {
    FlushCodecCtx(video_codec_ctx_, video_stream_);
  }

  if (audio_codec_ctx_) {
    FlushCodecCtx(audio_codec_ctx_, audio_stream_);
  }
}

void FFmpegEncoder::FlushCodecCtx(AVCodecContext *codec_ctx, AVStream* stream)
{
  avcodec_send_frame(codec_ctx, nullptr);
  AVPacket* pkt = av_packet_alloc();

  int error_code;
  do {
    error_code = avcodec_receive_packet(codec_ctx, pkt);

    if (error_code < 0) {
      break;
    }

    pkt->stream_index = stream->index;
    av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);
    av_interleaved_write_frame(fmt_ctx_, pkt);
    av_packet_unref(pkt);
  } while (error_code >= 0);

  av_packet_free(&pkt);
}

void FFmpegEncoder::Error(const QString &s)
{
  qWarning() << s;

  Close();
}

OLIVE_NAMESPACE_EXIT
