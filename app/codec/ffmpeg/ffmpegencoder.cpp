#include "ffmpegencoder.h"

#include <QFile>

#include "ffmpegcommon.h"
#include "render/pixelformat.h"

FFmpegEncoder::FFmpegEncoder(const EncodingParams &params) :
  Encoder(params),
  fmt_ctx_(nullptr),
  video_stream_(nullptr),
  video_codec_ctx_(nullptr),
  video_scale_ctx_(nullptr),
  audio_stream_(nullptr),
  audio_codec_ctx_(nullptr),
  audio_resample_ctx_(nullptr)
{
}

void FFmpegEncoder::WriteAudio(const AudioRenderingParams &pcm_info, const QString &pcm_filename, const TimeRange& range)
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
        // If no video, just use an arbitary number
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

    // Set up frame
    frame->channel_layout = audio_codec_ctx_->channel_layout;
    frame->nb_samples = maximum_frame_samples;
    frame->format = audio_codec_ctx_->sample_fmt;

    // Allocate its buffers
    av_frame_get_buffer(frame, 0);
    int sample_counter = 0;

    // Go to start range if one was set
    if (range.in() > 0) {
      pcm.seek(pcm_info.time_to_bytes(range.in()));
    }

    int end_in_bytes = pcm_info.time_to_bytes(range.out());

    while (true) {
      int samples_needed = static_cast<int>(frame->nb_samples + swr_get_delay(swr_ctx, pcm_info.sample_rate()));

      int max_read = pcm_info.samples_to_bytes(samples_needed);

      // Limit read to end if necessary
      if (pcm.pos() + max_read > end_in_bytes) {
        max_read = end_in_bytes - pcm.pos();
      }

      QByteArray input_data;
      input_data = pcm.read(max_read);
      const char* input_data_array = input_data.constData();
      samples_needed = pcm_info.bytes_to_samples(input_data.size());

      // Use swresample to convert the data into the correct format/linesize
      int converted = swr_convert(swr_ctx,
                                  frame->data,
                                  maximum_frame_samples,
                                  reinterpret_cast<const uint8_t**>(&input_data_array),
                                  samples_needed);

      frame->pts = sample_counter;

      sample_counter += converted;

      WriteAVFrame(frame, audio_codec_ctx_, audio_stream_);

      if (pcm.atEnd() || pcm.pos() == end_in_bytes) {
        break;
      }
    }

    av_frame_free(&frame);

    swr_free(&swr_ctx);

    pcm.close();
  }

  emit AudioComplete();
}

bool FFmpegEncoder::OpenInternal()
{
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

  return true;
}

void FFmpegEncoder::WriteInternal(FramePtr frame)
{
  AVFrame* encoded_frame = av_frame_alloc();

  int error_code;
  const char* input_data;
  int input_linesize;

  // Frame must be video
  encoded_frame->width = frame->width();
  encoded_frame->height = frame->height();
  encoded_frame->format = video_codec_ctx_->pix_fmt;

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
  input_linesize = frame->width() * PixelFormat::BytesPerPixel(video_conversion_fmt_);
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

  encoded_frame->pts = qRound(frame->timestamp().toDouble() / av_q2d(video_codec_ctx_->time_base));

  WriteAVFrame(encoded_frame, video_codec_ctx_, video_stream_);

fail:
  av_frame_free(&encoded_frame);
}

void FFmpegEncoder::CloseInternal()
{
  if (IsOpen()) {
    // Flush encoders
    FlushEncoders();

    // We've written a header, so we'll write a trailer
    av_write_trailer(fmt_ctx_);
    avio_closep(&fmt_ctx_->pb);
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

void FFmpegEncoder::WriteAVFrame(AVFrame *frame, AVCodecContext* codec_ctx, AVStream* stream)
{
  // Send raw frame to the encoder
  int error_code = avcodec_send_frame(codec_ctx, frame);
  if (error_code < 0) {
    FFmpegError("Failed to send frame to encoder", error_code);
    return;
  }

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

fail:
  av_packet_free(&pkt);
}

bool FFmpegEncoder::InitializeStream(AVMediaType type, AVStream** stream_ptr, AVCodecContext** codec_ctx_ptr, const QString& codec)
{
  if (type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO) {
    Error(QStringLiteral("Cannot initialize a stream that is not a video or audio type"));
    return false;
  }

  // Retrieve codec and convert to C string
  QByteArray codec_bytes = codec.toUtf8();
  const char* codec_c_str = codec_bytes.constData();

  // Find encoder with this name
  AVCodec* encoder = avcodec_find_encoder_by_name(codec_c_str);

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
    codec_ctx->sample_aspect_ratio = {1, 1};
    codec_ctx->time_base = params().video_params().time_base().toAVRational();

    // FIXME: Make this customizable again
    codec_ctx->pix_fmt = encoder->pix_fmts[0];

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
  av_dict_set(&codec_opts, "threads", "auto", 0);

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
