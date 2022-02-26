/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "tempoprocessor.h"

extern "C" {
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
}

#include <QDebug>

#include "common/ffmpegutils.h"

namespace olive {

TempoProcessor::TempoProcessor() :
  filter_graph_(nullptr),
  buffersrc_ctx_(nullptr),
  buffersink_ctx_(nullptr),
  open_(false)
{
}

TempoProcessor::~TempoProcessor()
{
  Close();
}

bool TempoProcessor::IsOpen() const
{
  return open_;
}

const double &TempoProcessor::GetSpeed() const
{
  return speed_;
}

bool TempoProcessor::Open(const AudioParams &params, const double& speed)
{
  if (open_) {
    return true;
  }

  params_ = params;
  speed_ = speed;

  // Create AVFilterGraph instance
  filter_graph_ = avfilter_graph_alloc();
  if (!filter_graph_) {
    qCritical() << "Failed to create AVFilterGraph";
    Close();
    return false;
  }

  // Set up audio buffer args
  char filter_args[200];
  snprintf(filter_args, 200, "time_base=%d/%d:sample_rate=%d:sample_fmt=%d:channel_layout=0x%" PRIx64,
           1,
           params_.sample_rate(),
           params_.sample_rate(),
           FFmpegUtils::GetFFmpegSampleFormat(params_.format()),
           params.channel_layout());

  // Create buffer and buffersink
  if (avfilter_graph_create_filter(&buffersrc_ctx_, avfilter_get_by_name("abuffer"), "in", filter_args, nullptr, filter_graph_) < 0) {
    qCritical() << "Failed to create audio buffer source";
    Close();
    return false;
  }

  if (avfilter_graph_create_filter(&buffersink_ctx_, avfilter_get_by_name("abuffersink"), "out", nullptr, nullptr, filter_graph_) < 0) {
    qCritical() << "Failed to create audio buffer sink";
    Close();
    return false;
  }

  // Create audio tempo filters: FFmpeg's atempo can only be set between 0.5 and 2.0. If the requested speed is outside
  // those boundaries, we need to daisychain more than one together.
  double base = (speed_ > 1.0) ? 2.0 : 0.5;
  double speed_log = log(speed_) / log(base);

  // This is the number of how many 0.5 or 2.0 tempos we need to daisychain
  int whole = qFloor(speed_log);

  // Set speed_log to the remainder
  speed_log -= whole;

  AVFilterContext* previous_filter = buffersrc_ctx_;

  for (int i=0;i<=whole;i++) {
    double filter_tempo = (i == whole) ? qPow(base, speed_log) : base;

    if (qFuzzyCompare(filter_tempo, 1.0)) {
      // This filter would do nothing
      continue;
    }

    previous_filter = CreateTempoFilter(filter_graph_,
                                        previous_filter,
                                        filter_tempo);

    if (!previous_filter) {
      qCritical() << "Failed to create audio tempo filter";
      Close();
      return false;
    }
  }

  // Link the last filter to the buffersink
  if (avfilter_link(previous_filter, 0, buffersink_ctx_, 0) != 0) {
    qCritical() << "Failed to link final filter and buffer sink";
    Close();
    return false;
  }

  // Config graph
  if (avfilter_graph_config(filter_graph_, nullptr) < 0) {
    qCritical() << "Failed to configure filter graph";
    Close();
    return false;
  }

  timestamp_ = 0;

  open_ = true;

  flushed_ = false;

  return true;
}

void TempoProcessor::Push(const QByteArray &packed)
{
  if (!IsOpen()) {
    qWarning() << "Tried to push to closed TempoProcessor";
    return;
  }

  if (flushed_) {
    qWarning() << "Tried to push to flushed TempoProcessor";
    return;
  }

  AVFrame* src_frame = av_frame_alloc();

  if (!src_frame) {
    qCritical() << "Failed to allocate source frame";
    return;
  }

  // Allocate a buffer for the number of samples we got
  src_frame->sample_rate = params_.sample_rate();
  src_frame->format = FFmpegUtils::GetFFmpegSampleFormat(params_.format());
  src_frame->channel_layout = params_.channel_layout();
  src_frame->nb_samples = params_.bytes_to_samples(packed.size());
  src_frame->pts = timestamp_;
  timestamp_ += src_frame->nb_samples;

  if (av_frame_get_buffer(src_frame, 0) < 0) {
    qCritical() << "Failed to allocate buffer for source frame";
    av_frame_free(&src_frame);
    return;
  }

  // Copy buffer from data array to frame
  memcpy(src_frame->data[0], packed, packed.size());

  int ret = av_buffersrc_add_frame_flags(buffersrc_ctx_, src_frame, AV_BUFFERSRC_FLAG_KEEP_REF);

  if (ret < 0) {
    qCritical() << "Failed to feed buffer source" << ret;
  }

  if (src_frame) {
    av_frame_free(&src_frame);
  }
}

void TempoProcessor::Flush()
{
  if (!flushed_) {
    int ret = av_buffersrc_add_frame_flags(buffersrc_ctx_, nullptr, AV_BUFFERSRC_FLAG_KEEP_REF);
    if (ret < 0) {
      qCritical() << "Failed to feed buffer source" << ret;
    }
    flushed_ = true;
  }
}

QByteArray TempoProcessor::Pull()
{
  QByteArray b;
  AVFrame *processed_frame = av_frame_alloc();

  // Try to pull samples from the buffersink
  int ret = av_buffersink_get_frame(buffersink_ctx_, processed_frame);

  if (ret < 0) {
    // We couldn't pull for some reason, if the error was EAGAIN, we just need to send more samples. Otherwise the
    // error might be fatal...
    if (ret != AVERROR(EAGAIN)) {
      qCritical() << "Failed to pull from buffersink" << ret;
    }

    av_frame_free(&processed_frame);
    return b;
  }

  b.resize(params_.samples_to_bytes(processed_frame->nb_samples));

  // Copy the bytes
  memcpy(b.data(), processed_frame->data[0], b.size());

  // If the index has reached the limit of this processed frame, we can dispose of the frame now
  av_frame_free(&processed_frame);

  return b;
}

void TempoProcessor::Close()
{
  open_ = false;

  if (filter_graph_) {
    avfilter_graph_free(&filter_graph_);
    filter_graph_ = nullptr;
  }

  buffersrc_ctx_ = nullptr;
  buffersink_ctx_ = nullptr;
}

AVFilterContext *TempoProcessor::CreateTempoFilter(AVFilterGraph* graph, AVFilterContext* link, const double &tempo)
{
  // Set up tempo param, which is taken as a C string
  char speed_param[20];
  snprintf(speed_param, 20, "%f", tempo);

  AVFilterContext* tempo_ctx = nullptr;

  if (avfilter_graph_create_filter(&tempo_ctx, avfilter_get_by_name("atempo"), "atempo", speed_param, nullptr, graph) >= 0
      && avfilter_link(link, 0, tempo_ctx, 0) == 0) {
    return tempo_ctx;
  }

  return nullptr;
}

}
