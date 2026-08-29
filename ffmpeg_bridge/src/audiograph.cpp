/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team
  Modifications Copyright (C) 2025 mikesolar

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

#include "internal.h"

#include <math.h>
#include <stdio.h>

struct FBAudioGraph {
	AVFilterGraph *graph = nullptr;
	AVFilterContext *buffersrc = nullptr;
	AVFilterContext *buffersink = nullptr;
	AVFrame *in_frame = nullptr;

	int in_channels = 0;
	int in_sample_rate = 0;
	int in_sample_format = FB_SAMPLE_FMT_NONE;
	int64_t in_pts = 0;
};

static AVFilterContext *CreateTempoFilter(AVFilterGraph *graph,
										  AVFilterContext *link, double tempo)
{
	char speed_param[20];
	snprintf(speed_param, sizeof(speed_param), "%f", tempo);

	AVFilterContext *tempo_ctx = nullptr;
	if (avfilter_graph_create_filter(&tempo_ctx, avfilter_get_by_name("atempo"),
									 "atempo", speed_param, nullptr, graph) >= 0 &&
		avfilter_link(link, 0, tempo_ctx, 0) == 0) {
		return tempo_ctx;
	}

	return nullptr;
}

FBAudioGraph *fb_audio_graph_create(const FBAudioGraphConfig *config)
{
	if (!config) {
		return nullptr;
	}

	FBAudioGraph *g = new FBAudioGraph;

	g->graph = avfilter_graph_alloc();
	if (!g->graph) {
		delete g;
		return nullptr;
	}

	AVChannelLayout in_layout, out_layout;
	fb::ChannelLayoutFromMask(&in_layout, config->in_channel_layout_mask,
							  config->in_channels);
	fb::ChannelLayoutFromMask(&out_layout, config->out_channel_layout_mask,
							  config->out_channels);

	char filter_args[200];

	// Create buffersrc (input)
	snprintf(filter_args, sizeof(filter_args),
			 "time_base=1/%d:sample_rate=%d:sample_fmt=%d:channel_layout=0x%" PRIx64,
			 config->in_sample_rate, config->in_sample_rate,
			 config->in_sample_format, in_layout.u.mask);

	int r = avfilter_graph_create_filter(&g->buffersrc,
										 avfilter_get_by_name("abuffer"), "in",
										 filter_args, nullptr, g->graph);
	if (r < 0) {
		av_channel_layout_uninit(&in_layout);
		av_channel_layout_uninit(&out_layout);
		fb_audio_graph_free(&g);
		return nullptr;
	}

	AVFilterContext *previous_filter = g->buffersrc;

	// Create tempo filter chain: FFmpeg's atempo can only be set between 0.5
	// and 2.0, so out-of-range speeds must be daisychained.
	bool create_tempo = config->tempo != 1.0;
	if (create_tempo) {
		double base = (config->tempo > 1.0) ? 2.0 : 0.5;
		double speed_log = log(config->tempo) / log(base);
		int whole = int(floor(speed_log));
		speed_log -= whole;

		for (int i = 0; i <= whole; i++) {
			double filter_tempo = (i == whole) ? pow(base, speed_log) : base;
			previous_filter =
				CreateTempoFilter(g->graph, previous_filter, filter_tempo);
			if (!previous_filter) {
				av_channel_layout_uninit(&in_layout);
				av_channel_layout_uninit(&out_layout);
				fb_audio_graph_free(&g);
				return nullptr;
			}
		}
	}

	// Create conversion filter if the parameters differ (or if the tempo
	// filter converted planar input to packed and planar output is desired)
	if (config->in_sample_rate != config->out_sample_rate ||
		av_channel_layout_compare(&in_layout, &out_layout) != 0 ||
		config->in_sample_format != config->out_sample_format ||
		(config->out_is_planar && create_tempo)) {
		snprintf(filter_args, sizeof(filter_args),
				 "sample_fmts=%s:sample_rates=%d:channel_layouts=0x%" PRIx64,
				 av_get_sample_fmt_name(
					 static_cast<AVSampleFormat>(config->out_sample_format)),
				 config->out_sample_rate, out_layout.u.mask);

		AVFilterContext *c;
		r = avfilter_graph_create_filter(&c, avfilter_get_by_name("aformat"),
										 "fmt", filter_args, nullptr, g->graph);
		if (r < 0 || avfilter_link(previous_filter, 0, c, 0) < 0) {
			av_channel_layout_uninit(&in_layout);
			av_channel_layout_uninit(&out_layout);
			fb_audio_graph_free(&g);
			return nullptr;
		}

		previous_filter = c;
	}

	// Create buffersink (output)
	r = avfilter_graph_create_filter(&g->buffersink,
									 avfilter_get_by_name("abuffersink"), "out",
									 nullptr, nullptr, g->graph);
	if (r < 0 || avfilter_link(previous_filter, 0, g->buffersink, 0) < 0 ||
		avfilter_graph_config(g->graph, nullptr) < 0) {
		av_channel_layout_uninit(&in_layout);
		av_channel_layout_uninit(&out_layout);
		fb_audio_graph_free(&g);
		return nullptr;
	}

	// Allocate the input frame used for pushes
	g->in_frame = av_frame_alloc();
	if (!g->in_frame) {
		av_channel_layout_uninit(&in_layout);
		av_channel_layout_uninit(&out_layout);
		fb_audio_graph_free(&g);
		return nullptr;
	}
	g->in_frame->sample_rate = config->in_sample_rate;
	g->in_frame->format = config->in_sample_format;
	g->in_frame->ch_layout = in_layout;
	g->in_frame->pts = 0;

	av_channel_layout_uninit(&out_layout);

	g->in_channels = in_layout.nb_channels;
	g->in_sample_rate = config->in_sample_rate;
	g->in_sample_format = config->in_sample_format;

	return g;
}

void fb_audio_graph_free(FBAudioGraph **graph)
{
	if (graph && *graph) {
		FBAudioGraph *g = *graph;
		if (g->graph) {
			avfilter_graph_free(&g->graph);
		}
		if (g->in_frame) {
			// in_frame owns a reference to its ch_layout (copied at create)
			av_channel_layout_uninit(&g->in_frame->ch_layout);
			av_frame_free(&g->in_frame);
		}
		delete g;
		*graph = nullptr;
	}
}

int fb_audio_graph_push(FBAudioGraph *graph,
						const uint8_t *const *channel_data, int nb_samples)
{
	if (!graph) {
		return AVERROR(EINVAL);
	}

	if (channel_data && nb_samples > 0) {
		int bytes_per_sample =
			av_get_bytes_per_sample(static_cast<AVSampleFormat>(graph->in_sample_format));

		graph->in_frame->nb_samples = nb_samples;
		for (int i = 0; i < graph->in_channels; i++) {
			graph->in_frame->data[i] =
				const_cast<uint8_t *>(channel_data[i]);
			graph->in_frame->linesize[i] = bytes_per_sample * nb_samples;
		}

		int r = av_buffersrc_add_frame_flags(graph->buffersrc, graph->in_frame,
											 AV_BUFFERSRC_FLAG_KEEP_REF);
		if (r < 0) {
			return r;
		}
	} else {
		// Flush
		int r = av_buffersrc_add_frame_flags(graph->buffersrc, nullptr,
											 AV_BUFFERSRC_FLAG_KEEP_REF);
		if (r < 0) {
			return r;
		}
	}

	return 0;
}

int fb_audio_graph_pull(FBAudioGraph *graph, FBFrame *out_frame)
{
	if (!graph || !out_frame) {
		return AVERROR(EINVAL);
	}

	fb_frame_unref(out_frame);
	int r = av_buffersink_get_frame(graph->buffersink, out_frame->frame);
	if (r < 0) {
		if (r == AVERROR(EAGAIN)) {
			return 0;
		}
		return r;
	}

	return 1;
}
