/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#include "exportthread.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <QApplication>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QtMath>

#include "global/global.h"
#include "timeline/sequence.h"
#include "panels/panels.h"
#include "ui/viewerwidget.h"
#include "rendering/renderthread.h"
#include "rendering/renderfunctions.h"
#include "rendering/audio.h"
#include "ui/mainwindow.h"
#include "global/debug.h"

ExportThread::ExportThread(const ExportParams &params,
                           const VideoCodecParams& vparams,
                           QObject *parent) :
  QThread(parent),
  params_(params),
  vcodec_params_(vparams),
  interrupt_(false),
  fmt_ctx(nullptr),
  video_stream(nullptr),
  vcodec(nullptr),
  vcodec_ctx(nullptr),
  video_frame(nullptr),
  sws_ctx(nullptr),
  audio_stream(nullptr),
  acodec(nullptr),
  audio_frame(nullptr),
  swr_frame(nullptr),
  acodec_ctx(nullptr),
  swr_ctx(nullptr),
  vpkt_alloc(false),
  apkt_alloc(false),
  c_filename(nullptr)
{
  // Create offscreen surface for rendering while exporting
  surface.create();
}

bool ExportThread::Encode(AVFormatContext* ofmt_ctx, AVCodecContext* codec_ctx, AVFrame* frame, AVPacket* packet, AVStream* stream) {
  ret = avcodec_send_frame(codec_ctx, frame);
  if (ret < 0) {
    qCritical() << "Failed to send frame to encoder." << ret;
    export_error = tr("failed to send frame to encoder (%1)").arg(QString::number(ret));
    return false;
  }

  while (ret >= 0) {
    ret = avcodec_receive_packet(codec_ctx, packet);
    if (ret == AVERROR(EAGAIN)) {
      return true;
    } else if (ret < 0) {
      if (ret != AVERROR_EOF) {
        qCritical() << "Failed to receive packet from encoder." << ret;
        export_error = tr("failed to receive packet from encoder (%1)").arg(QString::number(ret));
      }
      return false;
    }

    packet->stream_index = stream->index;

    av_packet_rescale_ts(packet, codec_ctx->time_base, stream->time_base);

    av_interleaved_write_frame(ofmt_ctx, packet);
    av_packet_unref(packet);
  }
  return true;
}

bool ExportThread::SetupVideo() {
  // if video is disabled, no setup necessary
  if (!params_.video_enabled) return true;

  // find video encoder
  vcodec = avcodec_find_encoder(static_cast<enum AVCodecID>(params_.video_codec));
  if (!vcodec) {
    qCritical() << "Could not find video encoder";
    export_error = tr("could not video encoder for %1").arg(QString::number(params_.video_codec));
    return false;
  }

  // create video stream
  video_stream = avformat_new_stream(fmt_ctx, vcodec);
  video_stream->id = 0;
  if (!video_stream) {
    qCritical() << "Could not allocate video stream";
    export_error = tr("could not allocate video stream");
    return false;
  }

  // allocate context
  //	vcodec_ctx = video_stream->codec;
  vcodec_ctx = avcodec_alloc_context3(vcodec);
  if (!vcodec_ctx) {
    qCritical() << "Could not allocate video encoding context";
    export_error = tr("could not allocate video encoding context");
    return false;
  }

  // setup context
  vcodec_ctx->codec_id = static_cast<enum AVCodecID>(params_.video_codec);
  vcodec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
  vcodec_ctx->width = params_.video_width;
  vcodec_ctx->height = params_.video_height;
  vcodec_ctx->sample_aspect_ratio = {1, 1};
  vcodec_ctx->pix_fmt = static_cast<AVPixelFormat>(vcodec_params_.pix_fmt);
  vcodec_ctx->framerate = av_d2q(params_.video_frame_rate, INT_MAX);
  if (params_.video_compression_type == COMPRESSION_TYPE_CBR) {
    vcodec_ctx->bit_rate = qRound(params_.video_bitrate * 1000000);
  }
  vcodec_ctx->time_base = av_inv_q(vcodec_ctx->framerate);
  video_stream->time_base = vcodec_ctx->time_base;

  if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
    vcodec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  // Some codecs require special settings so we set that up here
  switch (vcodec_ctx->codec_id) {

  /// H.264 specific settings
  case AV_CODEC_ID_H264:
  case AV_CODEC_ID_H265:
    switch (params_.video_compression_type) {
    case COMPRESSION_TYPE_CFR:
      av_opt_set(vcodec_ctx->priv_data, "crf", QString::number(static_cast<int>(params_.video_bitrate)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
      break;
    }
    break;

  }

  // Set export to be multithreaded
  AVDictionary* opts = nullptr;
  if (vcodec_params_.threads == 0) {
    av_dict_set(&opts, "threads", "auto", 0);
  } else {
    av_dict_set(&opts, "threads", QString::number(vcodec_params_.threads).toUtf8(), 0);
  }

  // Open video encoder
  ret = avcodec_open2(vcodec_ctx, vcodec, &opts);
  if (ret < 0) {
    qCritical() << "Could not open output video encoder." << ret;
    export_error = tr("could not open output video encoder (%1)").arg(QString::number(ret));
    return false;
  }

  // Copy video encoder parameters to output stream
  ret = avcodec_parameters_from_context(video_stream->codecpar, vcodec_ctx);
  if (ret < 0) {
    qCritical() << "Could not copy video encoder parameters to output stream." << ret;
    export_error = tr("could not copy video encoder parameters to output stream (%1)").arg(QString::number(ret));
    return false;
  }

  // Create raw AVFrame that will contain the RGBA buffer straight from compositing
  video_frame = av_frame_alloc();
  av_frame_make_writable(video_frame);
  video_frame->format = AV_PIX_FMT_RGBA;
  video_frame->width = olive::ActiveSequence->width;
  video_frame->height = olive::ActiveSequence->height;
  av_frame_get_buffer(video_frame, 0);

  av_init_packet(&video_pkt);

  // Set up conversion context
  sws_ctx = sws_getContext(
        olive::ActiveSequence->width,
        olive::ActiveSequence->height,
        AV_PIX_FMT_RGBA,
        params_.video_width,
        params_.video_height,
        vcodec_ctx->pix_fmt,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr
        );

  return true;
}

bool ExportThread::SetupAudio() {

  // Find encoder for this codec
  acodec = avcodec_find_encoder(static_cast<AVCodecID>(params_.audio_codec));
  if (!acodec) {
    qCritical() << "Could not find audio encoder";
    export_error = tr("could not audio encoder for %1").arg(QString::number(params_.audio_codec));
    return false;
  }

  // Allocate audio stream
  audio_stream = avformat_new_stream(fmt_ctx, acodec);
  if (audio_stream == nullptr) {
    qCritical() << "Could not allocate audio stream";
    export_error = tr("could not allocate audio stream");
    return false;
  }

  // Set audio stream's ID to 1
  audio_stream->id = 1;

  // set sample rate to use for project
  audio_rendering_rate = params_.audio_sampling_rate;

  // Allocate encoding context
  acodec_ctx = avcodec_alloc_context3(acodec);
  if (!acodec_ctx) {
    qCritical() << "Could not find allocate audio encoding context";
    export_error = tr("could not allocate audio encoding context");
    return false;
  }

  // Set up encoding context
  acodec_ctx->codec_id = static_cast<AVCodecID>(params_.audio_codec);
  acodec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
  acodec_ctx->sample_rate = params_.audio_sampling_rate;
  acodec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;  // change this to support surround/mono sound in the future (this is what the user sets the output audio to)
  acodec_ctx->channels = av_get_channel_layout_nb_channels(acodec_ctx->channel_layout);
  acodec_ctx->sample_fmt = acodec->sample_fmts[0];
  acodec_ctx->bit_rate = params_.audio_bitrate * 1000;

  acodec_ctx->time_base.num = 1;
  acodec_ctx->time_base.den = params_.audio_sampling_rate;
  audio_stream->time_base = acodec_ctx->time_base;

  if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
    acodec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  // Open encoder
  ret = avcodec_open2(acodec_ctx, acodec, nullptr);
  if (ret < 0) {
    qCritical() << "Could not open output audio encoder." << ret;
    export_error = tr("could not open output audio encoder (%1)").arg(QString::number(ret));
    return false;
  }

  // Copy paramters from the codec context (set up above) to the output stream
  ret = avcodec_parameters_from_context(audio_stream->codecpar, acodec_ctx);
  if (ret < 0) {
    qCritical() << "Could not copy audio encoder parameters to output stream." << ret;
    export_error = tr("could not copy audio encoder parameters to output stream (%1)").arg(QString::number(ret));
    return false;
  }

  // init audio resampler context
  swr_ctx = swr_alloc_set_opts(
        nullptr,
        acodec_ctx->channel_layout,
        acodec_ctx->sample_fmt,
        acodec_ctx->sample_rate,
        olive::ActiveSequence->audio_layout,
        AV_SAMPLE_FMT_S16,
        acodec_ctx->sample_rate,
        0,
        nullptr
        );
  swr_init(swr_ctx);

  // initialize raw audio frame
  audio_frame = av_frame_alloc();
  audio_frame->sample_rate = acodec_ctx->sample_rate;
  audio_frame->nb_samples = acodec_ctx->frame_size;

  if (audio_frame->nb_samples == 0) {
    // FIXME: Magic number. I don't know what to put here and truthfully I don't even know if it matters.
    audio_frame->nb_samples = 256;
  }

  // TODO change this to support surround/mono sound in the future (this is whatever format they're held in the internal buffer)
  audio_frame->channel_layout = AV_CH_LAYOUT_STEREO;

  audio_frame->format = AV_SAMPLE_FMT_S16;
  audio_frame->channels = av_get_channel_layout_nb_channels(audio_frame->channel_layout);
  av_frame_make_writable(audio_frame);
  ret = av_frame_get_buffer(audio_frame, 0);
  if (ret < 0) {
    qCritical() << "Could not allocate audio buffer." << ret;
    export_error = tr("could not allocate audio buffer (%1)").arg(QString::number(ret));
    return false;
  }
  aframe_bytes = av_samples_get_buffer_size(nullptr, audio_frame->channels, audio_frame->nb_samples, static_cast<AVSampleFormat>(audio_frame->format), 0);

  av_init_packet(&audio_pkt);

  // init converted audio frame
  swr_frame = av_frame_alloc();
  swr_frame->channel_layout = acodec_ctx->channel_layout;
  swr_frame->channels = acodec_ctx->channels;
  swr_frame->sample_rate = acodec_ctx->sample_rate;
  swr_frame->format = acodec_ctx->sample_fmt;
  swr_frame->nb_samples = acodec_ctx->frame_size;
  av_frame_get_buffer(swr_frame, 0);

  av_frame_make_writable(swr_frame);

  return true;
}

bool ExportThread::SetupContainer() {

  // Set up output context (using the filename as the format specification)

  avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, c_filename);
  if (fmt_ctx == nullptr) {

    // Failed to create the output format context. Exit the export and throw an error.

    qCritical() << "Could not create output context";
    export_error = tr("could not create output format context");
    return false;
  }

  ret = avio_open(&fmt_ctx->pb, c_filename, AVIO_FLAG_WRITE);
  if (ret < 0) {

    // Failed to get a valid write handle for the exported file. Exit the export and throw an error.

    qCritical() << "Could not open output file." << ret;
    export_error = tr("could not open output file (%1)").arg(QString::number(ret));
    return false;
  }

  return true;
}

void ExportThread::Export()
{
  // Copy filename from QString to const char
  QByteArray ba = params_.filename.toUtf8();
  c_filename = new char[ba.size()+1];
  strcpy(c_filename, ba.data());

  // Set up file container
  if (!SetupContainer()) {
    return;
  }

  // If video is enabled, set it up in the container now
  if (params_.video_enabled && !SetupVideo()) {
    return;
  }

  // If audio is enabled, set it up in the container now
  if (params_.audio_enabled && !SetupAudio()) {
    return;
  }

  // Write the container header based on what's been set up above
  ret = avformat_write_header(fmt_ctx, nullptr);
  if (ret < 0) {

    // FFmpeg failed to write the header, so cancel the export and throw an error

    qCritical() << "Could not write output file header." << ret;
    export_error = tr("could not write output file header (%1)").arg(QString::number(ret));

    return;
  }

  // Count audio samples in file (used for calculating PTS)
  long file_audio_samples = 0;

  // Set up timing variables, used for determining rendering ETA
  qint64 frame_start_time, frame_time, avg_time, eta, total_time = 0;

  // Frame counters - used for generating encoding statistics (e.g. average frame time, ETA, etc.)
  long remaining_frames, frame_count = 1;

  // Use Sequence Viewer's render thread - TODO separate this into a new render thread for background rendering
  RenderThread* renderer = panel_sequence_viewer->viewer_widget->get_renderer();

  // Override connection from RenderThread
  disconnect(renderer, SIGNAL(ready()), panel_sequence_viewer->viewer_widget, SLOT(queue_repaint()));
  connect(renderer, SIGNAL(ready()), this, SLOT(wake()));

  // Lock mutex (used for synchronization with RenderThread)
  mutex.lock();

  // Loop from now (set to the beginning frame earlier) to the end of the frame
  while (olive::ActiveSequence->playhead <= params_.end_frame && !interrupt_) {

    // Start timing how long this frame will take
    frame_start_time = QDateTime::currentMSecsSinceEpoch();

    // If we're exporting audio, run compose_audio() which will write mixed audio to the internal audio buffer
    if (params_.audio_enabled) {
      olive::rendering::compose_audio(nullptr, olive::ActiveSequence.get(), 1, true);
    }

    // If we're exporting video, trigger a render on the RenderThread
    if (params_.video_enabled) {
      do {
        // TODO optimize by rendering the next frame while encoding the last
        renderer->start_render(nullptr, olive::ActiveSequence.get(), 1, nullptr, video_frame->data[0], video_frame->linesize[0]/4);

        // Wait for RenderThread to return
        waitCond.wait(&mutex);

        if (interrupt_) {
          return;
        }

        // If the RenderThread failed, do another render
      } while (renderer->did_texture_fail());

      if (interrupt_) {
        return;
      }

    }

    // Get the current sequence playhead in seconds (used for timestamp calculations later on)
    double timecode_secs = double(olive::ActiveSequence->playhead - params_.start_frame) / olive::ActiveSequence->frame_rate;

    // If we're exporting video, construct an AVFrame in the destination codec's pixel format to convert the raw RGBA
    // OpenGL buffer to
    if (params_.video_enabled) {

      //
      // - I'm not sure why, but we have to alloc/free sws_frame every frame, or it breaks GIF exporting.
      // - (i.e. GIFs get stuck on the first frame)
      // - The same problem/solution can be seen here: https://stackoverflow.com/a/38997739
      // - Perhaps this is the intended way to use swscale, but it seems inefficient.
      // - Anyway, here we are.
      //

      // Construct destination pixel format frame
      sws_frame = av_frame_alloc();
      sws_frame->format = vcodec_ctx->pix_fmt;
      sws_frame->width = params_.video_width;
      sws_frame->height = params_.video_height;
      av_frame_get_buffer(sws_frame, 0);

      // Convert raw RGBA buffer to format expected by the encoder
      sws_scale(sws_ctx, video_frame->data, video_frame->linesize, 0, video_frame->height, sws_frame->data, sws_frame->linesize);
      sws_frame->pts = qRound(timecode_secs/av_q2d(vcodec_ctx->time_base));

      // Send frame to encoder
      if (!Encode(fmt_ctx, vcodec_ctx, sws_frame, &video_pkt, video_stream)) {
        return;
      }

      av_frame_free(&sws_frame);
      sws_frame = nullptr;
    }

    // If we're exporting audio, copy audio from the buffer into an AVFrame for encoding
    if (params_.audio_enabled) {

      // Check if the count of encoded samples exceeds the current Sequence playhead, in which case we don't need to
      // encode any audio at this moment
      while (!interrupt_ && file_audio_samples <= (timecode_secs*params_.audio_sampling_rate)) {

        // Copy samples from audio buffer to AVFrame
        int adjusted_read = audio_ibuffer_read%audio_ibuffer_size;
        int copylen = qMin(aframe_bytes, audio_ibuffer_size-adjusted_read);
        memcpy(audio_frame->data[0], audio_ibuffer+adjusted_read, copylen);
        memset(audio_ibuffer+adjusted_read, 0, copylen);
        audio_ibuffer_read += copylen;

        // If we reached the end of the buffer without reaching the end of the frame, do another copy from the start
        // of the buffer
        if (copylen < aframe_bytes) {
          int remainder_len = aframe_bytes-copylen;
          memcpy(audio_frame->data[0]+copylen, audio_ibuffer, remainder_len);
          memset(audio_ibuffer, 0, remainder_len);
          audio_ibuffer_read += remainder_len;
        }

        // Convert raw audio samples to the destination codec's sample format
        swr_convert_frame(swr_ctx, swr_frame, audio_frame);

        // The timestamp is set to the current count of audio samples (since the audio stream's timebase is
        swr_frame->pts = file_audio_samples;

        // Send frame to encoder
        if (!Encode(fmt_ctx, acodec_ctx, swr_frame, &audio_pkt, audio_stream)) {
          return;
        }

        // Increment by the frame's number of samples
        file_audio_samples += swr_frame->nb_samples;
      }
    }

    // Generating encoding statistics (e.g. the time it took to encode this frame/estimated remaining time)
    frame_time = (QDateTime::currentMSecsSinceEpoch()-frame_start_time);
    total_time += frame_time;
    remaining_frames = (params_.end_frame - olive::ActiveSequence->playhead);
    avg_time = (total_time/frame_count);
    eta = (remaining_frames*avg_time);

    // Emit a signal for the percent of the sequence that's been encoded so far
    emit ProgressChanged(qRound((double(olive::ActiveSequence->playhead - params_.start_frame) / double(params_.end_frame - params_.start_frame)) * 100.0), eta);

    // Increment sequence playhead
    olive::ActiveSequence->playhead++;

    // Increment frame count (used for generating encoding statistics above)
    frame_count++;
  }

  // Restore original connection from RenderThread
  disconnect(renderer, SIGNAL(ready()), this, SLOT(wake()));
  connect(renderer, SIGNAL(ready()), panel_sequence_viewer->viewer_widget, SLOT(queue_repaint()));

  mutex.unlock();

  if (interrupt_) {
    return;
  }

  if (params_.video_enabled) vpkt_alloc = true;
  if (params_.audio_enabled) apkt_alloc = true;

  olive::Global->set_rendering_state(false);

  // If audio is enabled, flush the rest of the audio out of swresample
  if (params_.audio_enabled) {

    do {

      swr_convert_frame(swr_ctx, swr_frame, nullptr);
      if (swr_frame->nb_samples == 0) break;
      swr_frame->pts = file_audio_samples;
      if (!Encode(fmt_ctx, acodec_ctx, swr_frame, &audio_pkt, audio_stream)) {
        return;
      }
      file_audio_samples += swr_frame->nb_samples;

    } while (swr_frame->nb_samples > 0);

  }

  if (interrupt_) {
    return;
  }

  // Flush remaining packets out of video and audio encoders
  if (params_.video_enabled) {
    Encode(fmt_ctx, vcodec_ctx, nullptr, &video_pkt, video_stream);
  }
  if (params_.audio_enabled) {
    Encode(fmt_ctx, acodec_ctx, nullptr, &audio_pkt, audio_stream);
  }

  // Write container trailer
  ret = av_write_trailer(fmt_ctx);
  if (ret < 0) {
    qCritical() << "Could not write output file trailer." << ret;
    export_error = tr("could not write output file trailer (%1)").arg(QString::number(ret));
    return;
  }

  emit ProgressChanged(100, 0);
}

void ExportThread::Cleanup()
{
  if (fmt_ctx != nullptr) {
    avio_closep(&fmt_ctx->pb);
    avformat_free_context(fmt_ctx);
  }

  if (acodec_ctx != nullptr) {
    avcodec_close(acodec_ctx);
    avcodec_free_context(&acodec_ctx);
  }

  if (audio_frame != nullptr) {
    av_frame_free(&audio_frame);
  }

  if (apkt_alloc) {
    av_packet_unref(&audio_pkt);
  }

  if (vcodec_ctx != nullptr) {
    avcodec_close(vcodec_ctx);
    avcodec_free_context(&vcodec_ctx);
  }

  if (video_frame != nullptr) {
    av_frame_free(&video_frame);
  }

  if (vpkt_alloc) {
    av_packet_unref(&video_pkt);
  }

  if (sws_ctx != nullptr) {
    sws_freeContext(sws_ctx);
  }

  if (swr_ctx != nullptr) {
    swr_free(&swr_ctx);
  }

  if (swr_frame != nullptr) {
    av_frame_free(&swr_frame);
  }

  if (sws_frame != nullptr) {
    av_frame_free(&sws_frame);
  }

  delete [] c_filename;
}

void ExportThread::run() {
  // Ensure sequence isn't currently playing
  panel_sequence_viewer->pause();

  // Seek to the first frame we're exporting
  panel_sequence_viewer->seek(params_.start_frame);

  // Run export function (which will return if there's a failure)
  Export();

  // Clean up anything that was allocated in Export() (whether it succeeded or not)
  Cleanup();
}

const QString &ExportThread::GetError() {
  return export_error;
}

bool ExportThread::WasInterrupted()
{
  return interrupt_;
}

void ExportThread::Interrupt()
{
  interrupt_ = true;
}

void ExportThread::wake() {
  mutex.lock();
  waitCond.wakeAll();
  mutex.unlock();
}
