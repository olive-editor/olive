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

#include "audioprocessor.h"

extern "C" {
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

#include <QDebug>

#include "common/ffmpegutils.h"

namespace olive {

AudioProcessor::AudioProcessor()
{
  filter_graph_ = nullptr;
  in_frame_ = nullptr;
  out_frame_ = nullptr;
}

AudioProcessor::~AudioProcessor()
{
  Close();
}

bool AudioProcessor::Open(const AudioParams &from, const AudioParams &to, double tempo)
{
  if (filter_graph_) {
    qWarning() << "Tried to open a processor that was already open";
    return false;
  }

  filter_graph_ = avfilter_graph_alloc();
  if (!filter_graph_) {
    qCritical() << "Failed to allocate filter graph";
    return false;
  }

  from_fmt_ = FFmpegUtils::GetFFmpegSampleFormat(from.format());
  to_fmt_ = FFmpegUtils::GetFFmpegSampleFormat(to.format());

  // Set up audio buffer args
  char filter_args[200];
  snprintf(filter_args, 200, "time_base=%d/%d:sample_rate=%d:sample_fmt=%d:channel_layout=0x%" PRIx64,
           1,
           from.sample_rate(),
           from.sample_rate(),
           from_fmt_,
           from.channel_layout());

  int r;

  // Create buffersrc (input)
  r = avfilter_graph_create_filter(&buffersrc_ctx_, avfilter_get_by_name("abuffer"), "in", filter_args, nullptr, filter_graph_);
  if (r < 0) {
    qCritical() << "Failed to create buffersrc:" << r;
    Close();
    return false;
  }

  // Store "previous" filter for linking
  AVFilterContext *previous_filter = buffersrc_ctx_;

  // Create tempo
  bool create_tempo;
  if ((create_tempo = !qFuzzyCompare(tempo, 1.0))) {
    // Create audio tempo filters: FFmpeg's atempo can only be set between 0.5 and 2.0. If the requested speed is outside
    // those boundaries, we need to daisychain more than one together.
    double base = (tempo > 1.0) ? 2.0 : 0.5;
    double speed_log = log(tempo) / log(base);

    // This is the number of how many 0.5 or 2.0 tempos we need to daisychain
    int whole = std::floor(speed_log);

    // Set speed_log to the remainder
    speed_log -= whole;

    for (int i=0;i<=whole;i++) {
      double filter_tempo = (i == whole) ? std::pow(base, speed_log) : base;

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
  }

  // Create conversion filter
  if (from.sample_rate() != to.sample_rate() || from.channel_layout() != to.channel_layout() || from.format() != to.format()
      || (to.format().is_planar() && create_tempo)) { // Tempo processor automatically converts to packed,
                                                  // so if the desired output is planar, it'll need
                                                  // to be converted
    snprintf(filter_args, 200, "sample_fmts=%s:sample_rates=%d:channel_layouts=0x%" PRIx64,
             av_get_sample_fmt_name(to_fmt_),
             to.sample_rate(),
             to.channel_layout());

    AVFilterContext *c;
    r = avfilter_graph_create_filter(&c, avfilter_get_by_name("aformat"), "fmt", filter_args, nullptr, filter_graph_);
    if (r < 0) {
      qCritical() << "Failed to create format conversion filter:" << r << filter_args;
      Close();
      return false;
    }

    r = avfilter_link(previous_filter, 0, c, 0);
    if (r < 0) {
      qCritical() << "Failed to link filters:" << r;
      Close();
      return false;
    }

    previous_filter = c;
  }

  // Create buffersink (output)
  r = avfilter_graph_create_filter(&buffersink_ctx_, avfilter_get_by_name("abuffersink"), "out", nullptr, nullptr, filter_graph_);
  if (r < 0) {
    qCritical() << "Failed to create buffersink:" << r;
    Close();
    return false;
  }

  r = avfilter_link(previous_filter, 0, buffersink_ctx_, 0);
  if (r < 0) {
    qCritical() << "Failed to link filters:" << r;
    Close();
    return false;
  }

  r = avfilter_graph_config(filter_graph_, nullptr);
  if (r < 0) {
    qCritical() << "Failed to configure graph:" << r;
    Close();
    return false;
  }

  in_frame_ = av_frame_alloc();
  if (in_frame_) {
    in_frame_->sample_rate = from.sample_rate();
    in_frame_->format = from_fmt_;
    in_frame_->channel_layout = from.channel_layout();
    in_frame_->channels = from.channel_count();
    in_frame_->pts = 0;
  } else {
    qCritical() << "Failed to allocate input frame";
    Close();
    return false;
  }

  out_frame_ = av_frame_alloc();
  if (!out_frame_) {
    qCritical() << "Failed to allocate output frame";
    Close();
    return false;
  }

  from_ = from;
  to_ = to;

  return true;
}

void AudioProcessor::Close()
{
  if (filter_graph_) {
    avfilter_graph_free(&filter_graph_);
    filter_graph_ = nullptr;
    buffersrc_ctx_ = nullptr;
    buffersink_ctx_ = nullptr;
  }

  if (in_frame_) {
    av_frame_free(&in_frame_);
    in_frame_ = nullptr;
  }

  if (out_frame_) {
    av_frame_free(&out_frame_);
    out_frame_ = nullptr;
  }
}

int AudioProcessor::Convert(float **in, int nb_in_samples, AudioProcessor::Buffer *output)
{
  if (!IsOpen()) {
    qCritical() << "Tried to convert on closed processor";
    return -1;
  }

  int r = 0;

  if (in && nb_in_samples) {
    // Set frame parameters
    in_frame_->nb_samples = nb_in_samples;
    for (int i=0; i<from_.channel_count(); i++) {
      in_frame_->data[i] = reinterpret_cast<uint8_t*>(in[i]);
      in_frame_->linesize[i] = from_.samples_to_bytes(nb_in_samples);
    }

    r = av_buffersrc_add_frame_flags(buffersrc_ctx_, in_frame_, AV_BUFFERSRC_FLAG_KEEP_REF);
    if (r < 0) {
      qCritical() << "Failed to add frame to buffersrc:" << r;
      return r;
    }
  }

  if (output) {
    int nb_channels = to_.channel_count();

    if (to_.format().is_packed()) {
      nb_channels = 1;
    }

    AudioProcessor::Buffer &result = *output;
    result.resize(nb_channels);

    int byte_offset = 0;

    while (true) {
      av_frame_unref(out_frame_);
      r = av_buffersink_get_frame(buffersink_ctx_, out_frame_);
      if (r < 0) {
        if (r == AVERROR(EAGAIN)) {
          r = 0;
        } else {
          // Handle unexpected error
          qCritical() << "Failed to pull from buffersink:" << r;
        }
        break;
      }

      int nb_bytes = out_frame_->nb_samples * to_.bytes_per_sample_per_channel();
      if (to_.format().is_packed()) {
        nb_bytes *= to_.channel_count();
      }

      for (int i=0; i<nb_channels; i++) {
        result[i].resize(byte_offset + nb_bytes);
        memcpy(result[i].data() + byte_offset, out_frame_->data[i], nb_bytes);
      }
      byte_offset += nb_bytes;
    }
    av_frame_unref(out_frame_);
  }

  return r;
}

void AudioProcessor::Flush()
{
  int r = av_buffersrc_add_frame_flags(buffersrc_ctx_, nullptr, AV_BUFFERSRC_FLAG_KEEP_REF);
  if (r < 0) {
    qCritical() << "Failed to flush:" << r;
  }
}

AVFilterContext *AudioProcessor::CreateTempoFilter(AVFilterGraph* graph, AVFilterContext* link, const double &tempo)
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
