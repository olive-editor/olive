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

#include "ffmpegencoder.h"

extern "C" {
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/pixdesc.h>
}

#include <QFile>

#include "common/ffmpegutils.h"

namespace olive {

FFmpegEncoder::FFmpegEncoder(const EncodingParams &params) :
  Encoder(params),
  fmt_ctx_(nullptr),
  video_stream_(nullptr),
  video_codec_ctx_(nullptr),
  video_scale_ctx_(nullptr),
  video_buffersrc_ctx_(nullptr),
  video_buffersink_ctx_(nullptr),
  audio_stream_(nullptr),
  audio_codec_ctx_(nullptr),
  audio_resample_ctx_(nullptr),
  audio_frame_(nullptr),
  open_(false)
{
}

QStringList FFmpegEncoder::GetPixelFormatsForCodec(ExportCodec::Codec c) const
{
  QStringList pix_fmts;

  const AVCodec* codec_info = GetEncoder(c, SampleFormat::INVALID);

  if (codec_info) {
    for (int i=0; codec_info->pix_fmts[i]!=-1; i++) {
      if (FFmpegUtils::ConvertJPEGSpaceToRegularSpace(codec_info->pix_fmts[i]) != codec_info->pix_fmts[i]) {
        // This is a deprecated "JPEG" space, skip it
        continue;
      }

      const char* pix_fmt_name = av_get_pix_fmt_name(codec_info->pix_fmts[i]);
      pix_fmts.append(pix_fmt_name);
    }
  }

  return pix_fmts;
}

std::vector<SampleFormat> FFmpegEncoder::GetSampleFormatsForCodec(ExportCodec::Codec c) const
{
  std::vector<SampleFormat> f;

  if (c == ExportCodec::kCodecPCM) {
    // FFmpeg lists these as separate codecs so we need custom functionality here
    // We list signed 16 first because ExportDialog will always use the first element by default
    // (because first element is the "default" in tFFmpeg)
    f = {
      SampleFormat::S16,
      SampleFormat::U8,
      SampleFormat::S32,
      SampleFormat::S64,
      SampleFormat::F32,
      SampleFormat::F64
    };
  } else {
    const AVCodec* codec_info = GetEncoder(c, SampleFormat::INVALID);

    if (codec_info && codec_info->sample_fmts) {
      for (int i=0; codec_info->sample_fmts[i]!=-1; i++) {
        SampleFormat this_format = FFmpegUtils::GetNativeSampleFormat(static_cast<AVSampleFormat>(codec_info->sample_fmts[i]));
        if (this_format != SampleFormat::INVALID) {
          f.push_back(this_format);
        }
      }
    }
  }

  return f;
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
    FFmpegError(tr("Failed to allocate output context"), error_code);
    return false;
  }

  // Initialize a video stream if it's enabled
  if (params().video_enabled()) {
    if (!InitializeStream(AVMEDIA_TYPE_VIDEO, &video_stream_, &video_codec_ctx_, params().video_codec())) {
      return false;
    }

    // This is the format we will expect frames received in Write() to be in
    PixelFormat native_pixel_fmt = params().video_params().format();

    // This is the format we will need to convert the frame to for swscale to understand it
    video_conversion_fmt_ = FFmpegUtils::GetCompatiblePixelFormat(native_pixel_fmt);

    // This is the equivalent pixel format above as an AVPixelFormat that swscale can understand
    AVPixelFormat src_alpha_pix_fmt = FFmpegUtils::GetFFmpegPixelFormat(video_conversion_fmt_,
                                                                        VideoParams::kRGBAChannelCount);

    AVPixelFormat src_noalpha_pix_fmt = FFmpegUtils::GetFFmpegPixelFormat(video_conversion_fmt_,
                                                                          VideoParams::kRGBChannelCount);

    if (src_alpha_pix_fmt == AV_PIX_FMT_NONE || src_noalpha_pix_fmt == AV_PIX_FMT_NONE) {
      SetError(tr("Failed to find suitable pixel format for this buffer"));
      return false;
    }

    // This is the pixel format the encoder wants to encode to
    AVPixelFormat encoder_pix_fmt = video_codec_ctx_->pix_fmt;

    video_scale_ctx_ = avfilter_graph_alloc();
    if (!video_scale_ctx_) {
      return false;
    }

    static const int FILTER_ARG_SZ = 1024;
    char filter_args[FILTER_ARG_SZ];

    snprintf(filter_args, FILTER_ARG_SZ, "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             params().video_params().effective_width(),
             params().video_params().effective_height(),
             src_alpha_pix_fmt,
             params().video_params().time_base().numerator(),
             params().video_params().time_base().denominator(),
             params().video_params().pixel_aspect_ratio().numerator(),
             params().video_params().pixel_aspect_ratio().denominator());

    avfilter_graph_create_filter(&video_buffersrc_ctx_, avfilter_get_by_name("buffer"), "in", filter_args, nullptr, video_scale_ctx_);
    avfilter_graph_create_filter(&video_buffersink_ctx_, avfilter_get_by_name("buffersink"), "out", nullptr, nullptr, video_scale_ctx_);

    AVFilterContext *last_filter = video_buffersrc_ctx_;

    {
      // Set color range
      AVFilterContext* range_filter;

      snprintf(filter_args, FILTER_ARG_SZ, "in_range=full:out_range=%s",
               params().video_params().color_range() == VideoParams::kColorRangeFull ? "full" : "limited");

      avfilter_graph_create_filter(&range_filter, avfilter_get_by_name("scale"), "range", filter_args, nullptr, video_scale_ctx_);

      avfilter_link(last_filter, 0, range_filter, 0);
      last_filter = range_filter;
    }

    if (src_alpha_pix_fmt != encoder_pix_fmt) {
      // Transform pixel format
      AVFilterContext* format_filter;

      snprintf(filter_args, FILTER_ARG_SZ, "pix_fmts=%u", encoder_pix_fmt);

      avfilter_graph_create_filter(&format_filter, avfilter_get_by_name("format"), "format", filter_args, nullptr, video_scale_ctx_);

      avfilter_link(last_filter, 0, format_filter, 0);
      last_filter = format_filter;
    }

    avfilter_link(last_filter, 0, video_buffersink_ctx_, 0);

    if (avfilter_graph_config(video_scale_ctx_, nullptr) < 0) {
      SetError(tr("Failed to configure filter graph"));
      return false;
    }
  }

  // Initialize an audio stream if it's enabled
  if (params().audio_enabled()) {
    if (!InitializeStream(AVMEDIA_TYPE_AUDIO, &audio_stream_, &audio_codec_ctx_, params().audio_codec())) {
      return false;
    }
  }

  // Initialize a subtitle stream if it's enabled
  if (params().subtitles_enabled()) {
    if (!InitializeStream(AVMEDIA_TYPE_SUBTITLE, &subtitle_stream_, &subtitle_codec_ctx_, params().subtitles_codec())) {
      return false;
    }
  }

  av_dump_format(fmt_ctx_, 0, filename_c_str, 1);

  // Open output file for writing
  error_code = avio_open(&fmt_ctx_->pb, filename_c_str, AVIO_FLAG_WRITE);
  if (error_code < 0) {
    FFmpegError(tr("Failed to open IO context"), error_code);
    return false;
  }

  // Write header
  error_code = avformat_write_header(fmt_ctx_, nullptr);
  if (error_code < 0) {
    FFmpegError(tr("Failed to write format header"), error_code);
    return false;
  }

  open_ = true;
  return true;
}

bool FFmpegEncoder::WriteFrame(FramePtr frame, rational time)
{
  // We may need to convert this frame to a frame that swscale will understand
  if (frame->format() != video_conversion_fmt_) {
    frame = frame->convert(video_conversion_fmt_);
  }

  // Use swscale context to convert formats/linesizes
  AVFramePtr input_frame = CreateAVFramePtr(av_frame_alloc());
  input_frame->width = frame->width();
  input_frame->height = frame->height();
  input_frame->format = FFmpegUtils::GetFFmpegPixelFormat(frame->format(), frame->channel_count());
  input_frame->data[0] = reinterpret_cast<uint8_t*>(frame->data());
  input_frame->linesize[0] = frame->linesize_bytes();

  input_frame->color_primaries = video_codec_ctx_->color_primaries;
  input_frame->color_trc = video_codec_ctx_->color_trc;
  input_frame->colorspace = video_codec_ctx_->colorspace;
  input_frame->color_range = video_codec_ctx_->color_range;

  int r;
  r = av_buffersrc_add_frame_flags(video_buffersrc_ctx_, input_frame.get(), AV_BUFFERSRC_FLAG_KEEP_REF);
  if (r < 0) {
    FFmpegError(tr("Failed to add frame to filter graph"), r);
    return false;
  }

  AVFramePtr encoded_frame = CreateAVFramePtr(av_frame_alloc());
  r = av_buffersink_get_frame(video_buffersink_ctx_, encoded_frame.get());
  if (r < 0) {
    FFmpegError(tr("Failed to retrieve frame from buffer sink"), r);
    return false;
  }

  encoded_frame->pts = qRound64(time.toDouble() / av_q2d(video_codec_ctx_->time_base));

  return WriteAVFrame(encoded_frame.get(), video_codec_ctx_, video_stream_);
}

bool FFmpegEncoder::WriteAudio(const SampleBuffer &audio)
{
  if (!audio.is_allocated()) {
    return true;
  }

  bool result = true;

  size_t start = 0;
  size_t end = audio.sample_count();
  const size_t max_frame = 48000;

  while (result && start < end) {
    // Create input buffer
    uint8_t** input_data = nullptr;
    size_t input_sample_count = std::min(end - start, max_frame);
    int input_linesize;

    int r = av_samples_alloc_array_and_samples(&input_data, &input_linesize, audio.audio_params().channel_count(),
                                               input_sample_count, FFmpegUtils::GetFFmpegSampleFormat(audio.audio_params().format()), 0);

    if (r < 0) {
      FFmpegError(tr("Failed to allocate sample array"), r);
      return false;
    } else {
      int bpsc = audio.audio_params().bytes_per_sample_per_channel();
      for (int i=0; i<audio.audio_params().channel_count(); i++) {
        memcpy(input_data[i], audio.data(i) + start, input_sample_count * bpsc);
      }

      start += input_sample_count;
    }

    result = WriteAudioData(audio.audio_params().is_valid() ? audio.audio_params() : params().audio_params(), const_cast<const uint8_t**>(input_data), input_sample_count);

    if (input_data) {
      av_freep(&input_data[0]);
      av_freep(&input_data);
    }
  }

  return result;
}

bool FFmpegEncoder::WriteAudioData(const AudioParams &audio_params, const uint8_t **input_data, int input_sample_count)
{
  if (!InitializeResampleContext(audio_params)) {
    qCritical() << "Failed to initialize resample context";
    return false;
  }

  bool result = true;

  // Create output buffer
  int output_sample_count = input_sample_count ? swr_get_out_samples(audio_resample_ctx_, input_sample_count) : 102400;
  uint8_t** output_data = nullptr;
  int output_linesize;
  av_samples_alloc_array_and_samples(&output_data, &output_linesize, audio_stream_->codecpar->channels,
                                     output_sample_count, static_cast<AVSampleFormat>(audio_stream_->codecpar->format), 0);

  // Perform conversion
  int converted = swr_convert(audio_resample_ctx_, output_data, output_sample_count, const_cast<const uint8_t**>(input_data), input_sample_count);
  if (converted > 0) {
    // Split sample buffer into frames
    for (int i=0; i<converted; ) {
      int frame_remaining_samples = audio_max_samples_ - audio_frame_offset_;
      int converted_remaining_samples = converted - i;

      int copy_length = qMin(frame_remaining_samples, converted_remaining_samples);

      av_samples_copy(audio_frame_->data, output_data, audio_frame_offset_, i,
                      copy_length,
                      audio_frame_->channels, static_cast<AVSampleFormat>(audio_frame_->format));

      audio_frame_offset_ += copy_length;
      i += copy_length;

      if (audio_frame_offset_ == audio_max_samples_ || (i == converted && !input_data)) {
        // Got all the samples we needed, write the frame
        audio_frame_->pts = av_rescale_q(audio_write_count_, {1, audio_codec_ctx_->sample_rate}, audio_codec_ctx_->time_base);

        WriteAVFrame(audio_frame_, audio_codec_ctx_, audio_stream_);
        audio_write_count_ += audio_frame_offset_;
        audio_frame_offset_ = 0;
      }
    }
  } else if (converted < 0) {
    FFmpegError(tr("Failed to resample audio"), converted);
    result = false;
  }

  if (!input_data && audio_frame_offset_ > 0) {
    audio_frame_->nb_samples = audio_frame_offset_;
    audio_frame_->pts = av_rescale_q(audio_write_count_, {1, audio_codec_ctx_->sample_rate}, audio_codec_ctx_->time_base);
    WriteAVFrame(audio_frame_, audio_codec_ctx_, audio_stream_);
  }

  // Free buffers created
  if (output_data) {
    av_freep(&output_data[0]);
    av_freep(&output_data);
  }

  return result;
}

QString GetAssTime(const rational &time)
{
  int64_t total_centiseconds = qRound64(time.toDouble() * 100);

  int64_t cs = total_centiseconds % 100;
  int64_t ss = (total_centiseconds / 100) % 60;
  int64_t mm = (total_centiseconds / 6000) % 60;
  int64_t hh = total_centiseconds / 360000;

  return QStringLiteral("%1:%2:%3.%4").arg(
        QString::number(hh),
        QStringLiteral("%1").arg(mm, 2, 10, QLatin1Char('0')),
        QStringLiteral("%1").arg(ss, 2, 10, QLatin1Char('0')),
        QStringLiteral("%1").arg(cs, 2, 10, QLatin1Char('0'))
      );
}

bool FFmpegEncoder::WriteSubtitle(const SubtitleBlock *sub_block)
{
  QByteArray utf8_sub = sub_block->GetText().toUtf8();

  AVPacket *pkt = av_packet_alloc();

  pkt->stream_index = subtitle_stream_->index;
  pkt->data = (uint8_t *) utf8_sub.data();
  pkt->size = utf8_sub.size();
  pkt->pts = Timecode::time_to_timestamp(sub_block->in(), subtitle_codec_ctx_->time_base, Timecode::kFloor);
  pkt->duration = av_rescale_q(qRound64(sub_block->length().toDouble() * 1000), {1, 1000}, subtitle_codec_ctx_->time_base);
  pkt->dts = pkt->pts;
  av_packet_rescale_ts(pkt, subtitle_codec_ctx_->time_base, subtitle_stream_->time_base);

  int err = av_interleaved_write_frame(fmt_ctx_, pkt);
  bool ret = true;

  if (err < 0) {
    FFmpegError(tr("Failed to write interleaved packet"), err);
    ret = false;
  }

  av_packet_free(&pkt);

  return ret;
}

/*
void FFmpegEncoder::WriteAudio(AudioParams pcm_info, QIODevice* file)
{


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
    QByteArray input_data = file->read(max_read);

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
    if (file->atEnd()) {
      break;
    }
  }
}
*/

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

  if (audio_resample_ctx_) {
    swr_init(audio_resample_ctx_);
    audio_resample_ctx_ = nullptr;
  }

  if (audio_frame_) {
    av_frame_free(&audio_frame_);
    audio_frame_ = nullptr;
  }

  if (video_scale_ctx_) {
    avfilter_graph_free(&video_scale_ctx_);
    video_scale_ctx_ = nullptr;
    video_buffersrc_ctx_ = nullptr;
    video_buffersink_ctx_ = nullptr;
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
    video_stream_ = nullptr;
    audio_stream_ = nullptr;
  }
}

void FFmpegEncoder::FFmpegError(const QString& context, int error_code)
{
  char err[1024];
  av_strerror(error_code, err, 1024);

  QString formatted_err = tr("%1: %2 %3").arg(context, err, QString::number(error_code));
  qDebug() << formatted_err;
  SetError(formatted_err);
}

bool FFmpegEncoder::WriteAVFrame(AVFrame *frame, AVCodecContext* codec_ctx, AVStream* stream)
{
  // Send raw frame to the encoder
  int error_code = avcodec_send_frame(codec_ctx, frame);
  if (error_code < 0) {
    FFmpegError(tr("Failed to send frame to encoder"), error_code);
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
      FFmpegError(tr("Failed to receive packet from decoder"), error_code);
      goto fail;
    }

    // Set packet stream index
    pkt->stream_index = stream->index;

    av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);

    // Write packet to file
    error_code = av_interleaved_write_frame(fmt_ctx_, pkt);
    if (error_code < 0) {
      FFmpegError(tr("Failed to write interleaved packet"), error_code);
      goto fail;
    }

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
  if (type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO && type != AVMEDIA_TYPE_SUBTITLE) {
    SetError(tr("Cannot initialize a stream that is not a video, audio, or subtitle type"));
    return false;
  }

  // Find encoder
  const AVCodec* encoder = GetEncoder(codec, params().audio_params().format());
  if (!encoder) {
    SetError(tr("Failed to find codec for 0x%1").arg(codec, 16));
    return false;
  }

  if (encoder->type != type) {
    SetError(tr("Retrieved unexpected codec type %1 for codec %2").arg(QString::number(encoder->type), QString::number(codec)));
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
    codec_ctx->time_base = params().video_params().frame_rate_as_time_base().toAVRational();
    codec_ctx->framerate = params().video_params().frame_rate().toAVRational();
    codec_ctx->pix_fmt = av_get_pix_fmt(params().video_pix_fmt().toUtf8());
    codec_ctx->color_range = params().video_params().color_range() == VideoParams::kColorRangeFull ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;

    if (params().video_params().interlacing() != VideoParams::kInterlaceNone) {
      // FIXME: I actually don't know what these flags do, the documentation helpfully doesn't
      //        explain them at all. I hope using both of them is the right thing to do.
      codec_ctx->flags |= AV_CODEC_FLAG_INTERLACED_DCT | AV_CODEC_FLAG_INTERLACED_ME;


      if (params().video_params().interlacing() == VideoParams::kInterlacedTopFirst) {
        codec_ctx->field_order = AV_FIELD_TT;
      } else {
        codec_ctx->field_order = AV_FIELD_BB;

        if (codec == ExportCodec::kCodecH264 || codec == ExportCodec::kCodecH264rgb) {
          // For some reason, FFmpeg doesn't set libx264's bff flag so we have to do it ourselves
          av_opt_set(codec_ctx->priv_data, "x264opts", "bff=1", AV_OPT_SEARCH_CHILDREN);
        }
      }
    }

    // Set custom options
    {
      for (auto i=params().video_opts().begin();i!=params().video_opts().end();i++) {
        if (!i.key().startsWith(QStringLiteral("ove_"))) {
          av_opt_set(codec_ctx->priv_data, i.key().toUtf8(), i.value().toUtf8(), AV_OPT_SEARCH_CHILDREN);
        }
      }

      if (params().video_bit_rate() > 0) {
        codec_ctx->bit_rate = params().video_bit_rate();
      }

      if (params().video_min_bit_rate() > 0) {
        codec_ctx->rc_min_rate = params().video_min_bit_rate();
      }

      if (params().video_max_bit_rate() > 0) {
        codec_ctx->rc_max_rate = params().video_max_bit_rate();
      }

      if (params().video_buffer_size() > 0) {
        codec_ctx->rc_buffer_size = static_cast<int>(params().video_buffer_size());
      }

      // nclc tags. See https://ffmpeg.org/doxygen/4.0/pixfmt_8h.html#ad384ee5a840bafd73daef08e6d9cafe7
      // ffprobe -v error -show_format -show_streams "C:\Users\Tom\Documents\srgb correct tags.mov"
      if (params().color_transform().output().contains(QStringLiteral("sRGB"), Qt::CaseInsensitive)) {
        codec_ctx->color_primaries = AVCOL_PRI_BT709;
        codec_ctx->color_trc = AVCOL_TRC_IEC61966_2_1;
        codec_ctx->colorspace = AVCOL_SPC_BT709;
      } else { // Assume Rec.709
        codec_ctx->color_primaries = AVCOL_PRI_BT709;
        codec_ctx->color_trc = AVCOL_TRC_BT709;
        codec_ctx->colorspace = AVCOL_SPC_BT709;
      }
    }

  } else if (type == AVMEDIA_TYPE_AUDIO) {

    // Assume audio stream
    codec_ctx->sample_rate = params().audio_params().sample_rate();
    codec_ctx->channel_layout = params().audio_params().channel_layout();
    codec_ctx->channels = av_get_channel_layout_nb_channels(codec_ctx->channel_layout);
    codec_ctx->sample_fmt = FFmpegUtils::GetFFmpegSampleFormat(params().audio_params().format());
    codec_ctx->time_base = {1, codec_ctx->sample_rate};

    if (params().audio_bit_rate() > 0) {
      codec_ctx->bit_rate = params().audio_bit_rate();
    }

  } else if (type == AVMEDIA_TYPE_SUBTITLE) {

    codec_ctx->time_base = av_get_time_base_q();

    QByteArray ass_header = SubtitleParams::GenerateASSHeader().toUtf8();
    codec_ctx->subtitle_header = new uint8_t[ass_header.size()];
    memcpy(codec_ctx->subtitle_header, ass_header.constData(), ass_header.size());
    codec_ctx->subtitle_header_size = ass_header.size();

  }

  if (!SetupCodecContext(stream, codec_ctx, encoder)) {
    return false;
  }

  return true;
}

bool FFmpegEncoder::InitializeCodecContext(AVStream **stream, AVCodecContext **codec_ctx, const AVCodec* codec)
{
  *stream = avformat_new_stream(fmt_ctx_, nullptr);
  if (!(*stream)) {
    SetError(tr("Failed to allocate AVStream"));
    return false;
  }

  // Allocate a codec context
  *codec_ctx = avcodec_alloc_context3(codec);
  if (!(*codec_ctx)) {
    SetError(tr("Failed to allocate AVCodecContext"));
    return false;
  }

  return true;
}

bool FFmpegEncoder::SetupCodecContext(AVStream* stream, AVCodecContext* codec_ctx, const AVCodec* codec)
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
    FFmpegError(tr("Failed to open encoder"), error_code);
    return false;
  }

  // Copy context settings to codecpar object
  error_code = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
  if (error_code < 0) {
    FFmpegError(tr("Failed to copy codec parameters to stream"), error_code);
    return false;
  }

  if (codec->type == AVMEDIA_TYPE_VIDEO) {
    stream->avg_frame_rate = codec_ctx->framerate;
  }

  return true;
}

void FFmpegEncoder::FlushEncoders()
{
  if (video_codec_ctx_) {
    FlushCodecCtx(video_codec_ctx_, video_stream_);
  }

  if (audio_codec_ctx_) {
    WriteAudio(SampleBuffer());

    FlushCodecCtx(audio_codec_ctx_, audio_stream_);
  }

  if (fmt_ctx_) {
    if (fmt_ctx_->oformat->flags & AVFMT_ALLOW_FLUSH) {
      int r = av_interleaved_write_frame(fmt_ctx_, nullptr);
      if (r < 0) {
        FFmpegError(tr("Failed to write interleaved packet"), r);
      }
    }
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
    int r = av_interleaved_write_frame(fmt_ctx_, pkt);
    if (r < 0) {
      FFmpegError(tr("Failed to write interleaved packet"), r);
      break;
    }
    av_packet_unref(pkt);
  } while (error_code >= 0);

  av_packet_free(&pkt);
}

bool FFmpegEncoder::InitializeResampleContext(const AudioParams &audio)
{
  if (audio_resample_ctx_) {
    return true;
  }

  // Create resample context
  audio_resample_ctx_ = swr_alloc_set_opts(nullptr,
                                           static_cast<int64_t>(audio_codec_ctx_->channel_layout),
                                           audio_codec_ctx_->sample_fmt,
                                           audio_codec_ctx_->sample_rate,
                                           static_cast<int64_t>(audio.channel_layout()),
                                           FFmpegUtils::GetFFmpegSampleFormat(audio.format()),
                                           audio.sample_rate(),
                                           0,
                                           nullptr);
  if (!audio_resample_ctx_) {
    return false;
  }

  int err = swr_init(audio_resample_ctx_);
  if (err < 0) {
    FFmpegError(tr("Failed to create resampling context"), err);
    return false;
  }

  audio_max_samples_ = audio_codec_ctx_->frame_size;
  if (!audio_max_samples_) {
    // If not, use another frame size
    if (params().video_enabled()) {
      // If we're encoding video, use enough samples to cover roughly one frame of video
      audio_max_samples_ = params().audio_params().time_to_samples(params().video_params().frame_rate_as_time_base());
    } else {
      // If no video, just use an arbitrary number
      audio_max_samples_ = 256;
    }
  }

  audio_frame_ = av_frame_alloc();
  if (!audio_frame_) {
    return false;
  }

  audio_frame_->channel_layout = audio_codec_ctx_->channel_layout;
  audio_frame_->format = audio_codec_ctx_->sample_fmt;
  audio_frame_->nb_samples = audio_max_samples_;

  err = av_frame_get_buffer(audio_frame_, 0);
  if (err < 0) {
    FFmpegError(tr("Failed to create audio frame"), err);
    return false;
  }

  audio_frame_offset_ = 0;
  audio_write_count_ = 0;

  return true;
}

const AVCodec *FFmpegEncoder::GetEncoder(ExportCodec::Codec c, SampleFormat aformat)
{
  switch (c) {
  case ExportCodec::kCodecH264:
    return avcodec_find_encoder_by_name("libx264");
  case ExportCodec::kCodecH264rgb:
    return avcodec_find_encoder_by_name("libx264rgb");
  case ExportCodec::kCodecDNxHD:
    return avcodec_find_encoder(AV_CODEC_ID_DNXHD);
  case ExportCodec::kCodecProRes:
    return avcodec_find_encoder(AV_CODEC_ID_PRORES);
    case ExportCodec::kCodecCineform:
    return avcodec_find_encoder(AV_CODEC_ID_CFHD);
  case ExportCodec::kCodecH265:
    return avcodec_find_encoder(AV_CODEC_ID_HEVC);
  case ExportCodec::kCodecVP9:
    return avcodec_find_encoder(AV_CODEC_ID_VP9);
  case ExportCodec::kCodecAV1: {
    const AVCodec *encoder = avcodec_find_encoder_by_name("libsvtav1");
    if(!encoder)
      encoder = avcodec_find_encoder(AV_CODEC_ID_AV1);
    return encoder;
  }
  case ExportCodec::kCodecOpenEXR:
    return avcodec_find_encoder(AV_CODEC_ID_EXR);
  case ExportCodec::kCodecPNG:
    return avcodec_find_encoder(AV_CODEC_ID_PNG);
  case ExportCodec::kCodecTIFF:
    return avcodec_find_encoder(AV_CODEC_ID_TIFF);
  case ExportCodec::kCodecMP2:
    return avcodec_find_encoder(AV_CODEC_ID_MP2);
  case ExportCodec::kCodecMP3:
    return avcodec_find_encoder(AV_CODEC_ID_MP3);
  case ExportCodec::kCodecAAC:
    return avcodec_find_encoder(AV_CODEC_ID_AAC);
  case ExportCodec::kCodecPCM:
    switch (aformat) {
    case SampleFormat::INVALID:
    case SampleFormat::COUNT:
    case SampleFormat::U8P:
    case SampleFormat::S16P:
    case SampleFormat::S32P:
    case SampleFormat::S64P:
    case SampleFormat::F32P:
    case SampleFormat::F64P:
      break;
    case SampleFormat::U8:
      return avcodec_find_encoder(AV_CODEC_ID_PCM_U8);
    case SampleFormat::S16:
      return avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
    case SampleFormat::S32:
      return avcodec_find_encoder(AV_CODEC_ID_PCM_S32LE);
    case SampleFormat::S64:
      return avcodec_find_encoder(AV_CODEC_ID_PCM_S64LE);
    case SampleFormat::F32:
      return avcodec_find_encoder(AV_CODEC_ID_PCM_F32LE);
    case SampleFormat::F64:
      return avcodec_find_encoder(AV_CODEC_ID_PCM_F64LE);
    }
    break;
  case ExportCodec::kCodecFLAC:
    return avcodec_find_encoder(AV_CODEC_ID_FLAC);
  case ExportCodec::kCodecOpus:
    return avcodec_find_encoder(AV_CODEC_ID_OPUS);
  case ExportCodec::kCodecVorbis:
    return avcodec_find_encoder(AV_CODEC_ID_VORBIS);
  case ExportCodec::kCodecSRT:
    return avcodec_find_encoder(AV_CODEC_ID_SUBRIP);
  case ExportCodec::kCodecCount:
    // These are audio or invalid codecs and therefore have no pixel formats
    break;
  }

  return nullptr;
}

}
