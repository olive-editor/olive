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

#include <stdio.h>
#include <string.h>

struct FBProbe {
	AVFormatContext *fmt_ctx = nullptr;
};

namespace
{

constexpr int64_t kAnalyzeDurationUs = 5000000;
constexpr int64_t kProbeSizeBytes = 20000000;

void FillStreamInfo(const AVStream *s, int has_decoder, FBStreamInfo *out)
{
	const AVCodecParameters *par = s->codecpar;

	memset(out, 0, sizeof(*out));
	out->index = s->index;
	out->codec_type = par->codec_type;
	out->codec_id = par->codec_id;
	out->has_decoder = has_decoder;
	out->width = par->width;
	out->height = par->height;
	out->pixel_format = fb::PixFmtFromAV(AVPixelFormat(par->format));
	out->field_order = FB_FIELD_ORDER_UNKNOWN;
	out->color_range = par->color_range;
	out->sample_rate = par->sample_rate;
	out->sample_format = par->format;
	out->channel_layout_mask = fb::ValidateStreamChannelLayoutMask(s);
	out->start_time = s->start_time;
	out->duration = s->duration;
	out->time_base_num = s->time_base.num;
	out->time_base_den = s->time_base.den;
	out->avg_frame_rate_num = s->avg_frame_rate.num;
	out->avg_frame_rate_den = s->avg_frame_rate.den;
}

bool IsCancelled(FBCancelCallback cancel, void *userdata)
{
	return cancel && cancel(userdata);
}

} // namespace

FBProbe *fb_probe_create(void)
{
	return new FBProbe;
}

void fb_probe_free(FBProbe **probe)
{
	if (probe && *probe) {
		fb_probe_close(*probe);
		delete *probe;
		*probe = nullptr;
	}
}

int fb_probe_open(FBProbe *probe, const char *filename)
{
	if (!probe || !filename) {
		return AVERROR(EINVAL);
	}

	AVDictionary *format_opts = nullptr;
	av_dict_set_int(&format_opts, "analyzeduration", kAnalyzeDurationUs, 0);
	av_dict_set_int(&format_opts, "probesize", kProbeSizeBytes, 0);

	AVFormatContext *ctx = nullptr;
	int error_code = avformat_open_input(&ctx, filename, nullptr, &format_opts);
	av_dict_free(&format_opts);

	if (ctx) {
		ctx->probesize = kProbeSizeBytes;
		ctx->max_analyze_duration = kAnalyzeDurationUs;

		// Subtitle streams are read on demand, don't let them slow down probing
		for (unsigned int i = 0; i < ctx->nb_streams; i++) {
			AVStream *stream = ctx->streams[i];
			if (stream && stream->codecpar &&
				stream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) {
				stream->discard = AVDISCARD_ALL;
			}
		}
	}

	if (error_code != 0) {
		return error_code;
	}

	avformat_find_stream_info(ctx, nullptr);

	probe->fmt_ctx = ctx;
	return 0;
}

void fb_probe_close(FBProbe *probe)
{
	if (probe && probe->fmt_ctx) {
		avformat_close_input(&probe->fmt_ctx);
		probe->fmt_ctx = nullptr;
	}
}

int fb_probe_get_stream_count(const FBProbe *probe)
{
	if (!probe || !probe->fmt_ctx) {
		return 0;
	}
	return int(probe->fmt_ctx->nb_streams);
}

int fb_probe_get_stream_info(const FBProbe *probe, int stream_index,
							 FBStreamInfo *out)
{
	if (!probe || !probe->fmt_ctx || !out || stream_index < 0 ||
		stream_index >= int(probe->fmt_ctx->nb_streams)) {
		return AVERROR(EINVAL);
	}

	const AVStream *s = probe->fmt_ctx->streams[stream_index];
	int has_decoder = avcodec_find_decoder(s->codecpar->codec_id) != nullptr;
	FillStreamInfo(s, has_decoder, out);
	return 0;
}

int64_t fb_probe_get_duration(const FBProbe *probe)
{
	if (!probe || !probe->fmt_ctx) {
		return FB_NOPTS_VALUE;
	}
	return probe->fmt_ctx->duration;
}

int64_t fb_probe_get_start_time(const FBProbe *probe)
{
	if (!probe || !probe->fmt_ctx) {
		return FB_NOPTS_VALUE;
	}
	return probe->fmt_ctx->start_time;
}

int fb_probe_duration_from_bitrate(const FBProbe *probe)
{
	if (!probe || !probe->fmt_ctx) {
		return 0;
	}
	return probe->fmt_ctx->duration_estimation_method ==
		   AVFMT_DURATION_FROM_BITRATE;
}

int fb_probe_get_metadata(FBProbe *probe, int stream_index, const char *key,
						  char *buffer, int buffer_size)
{
	if (!probe || !probe->fmt_ctx || !key || !buffer || buffer_size <= 0) {
		return 0;
	}

	AVDictionary *metadata = nullptr;
	if (stream_index < 0) {
		metadata = probe->fmt_ctx->metadata;
	} else if (stream_index < int(probe->fmt_ctx->nb_streams)) {
		metadata = probe->fmt_ctx->streams[stream_index]->metadata;
	} else {
		return 0;
	}

	AVDictionaryEntry *entry =
		av_dict_get(metadata, key, nullptr, AV_DICT_IGNORE_SUFFIX);
	if (!entry) {
		return 0;
	}

	snprintf(buffer, size_t(buffer_size), "%s", entry->value);
	return 1;
}

int fb_probe_video_stream_details(const char *filename, int stream_index,
								  FBVideoStreamDetails *out,
								  int decode_full_duration,
								  FBCancelCallback cancel,
								  void *cancel_userdata)
{
	if (!filename || !out) {
		return AVERROR(EINVAL);
	}

	memset(out, 0, sizeof(*out));
	out->field_order = FB_FIELD_ORDER_PROGRESSIVE;
	out->pixel_aspect_num = 1;
	out->pixel_aspect_den = 1;
	out->decoded_duration = FB_NOPTS_VALUE;

	FBDecoder *decoder = fb_decoder_create();
	if (fb_decoder_open(decoder, filename, stream_index) < 0) {
		fb_decoder_free(&decoder);
		return AVERROR_EXTERNAL;
	}

	FBStreamInfo info;
	if (fb_decoder_get_stream_info(decoder, &info) == 0) {
		out->field_order = info.field_order;
		out->frame_rate_num = info.avg_frame_rate_num;
		out->frame_rate_den = info.avg_frame_rate_den;
	}

	FBPacket *pkt = fb_packet_alloc();
	FBFrame *frame = fb_frame_alloc();

	int ret = 0;

	// Read at least one frame to get more information about this video stream
	if (fb_decoder_get_frame(decoder, pkt, frame) >= 0) {
		fb_decoder_guess_sample_aspect_ratio(decoder, frame,
											 &out->pixel_aspect_num,
											 &out->pixel_aspect_den);
		fb_decoder_guess_frame_rate(decoder, frame, &out->frame_rate_num,
									&out->frame_rate_den);
	}

	ret = fb_decoder_get_frame(decoder, pkt, frame);
	if (ret == FB_ERROR_EOF) {
		// Only one frame exists: this is a still image
		out->is_still = 1;
	} else if (decode_full_duration) {
		// Decode until the end to determine the true duration
		int64_t last_ts = fb_frame_get_best_effort_timestamp(frame);
		while (fb_decoder_get_frame(decoder, pkt, frame) >= 0 &&
			   !IsCancelled(cancel, cancel_userdata)) {
			last_ts = fb_frame_get_best_effort_timestamp(frame);
		}
		out->decoded_duration = last_ts;
	}

	fb_frame_free(&frame);
	fb_packet_free(&pkt);
	fb_decoder_free(&decoder);

	return 0;
}

int fb_probe_audio_stream_duration(const char *filename, int stream_index,
								   int64_t *out_duration,
								   FBCancelCallback cancel,
								   void *cancel_userdata)
{
	if (!filename || !out_duration) {
		return AVERROR(EINVAL);
	}

	FBDecoder *decoder = fb_decoder_create();
	if (fb_decoder_open(decoder, filename, stream_index) < 0) {
		fb_decoder_free(&decoder);
		return AVERROR_EXTERNAL;
	}

	FBPacket *pkt = fb_packet_alloc();
	FBFrame *frame = fb_frame_alloc();

	int64_t duration = 0;
	do {
		duration = fb_frame_get_best_effort_timestamp(frame);
	} while (fb_decoder_get_frame(decoder, pkt, frame) >= 0 &&
			 !IsCancelled(cancel, cancel_userdata));

	fb_frame_free(&frame);
	fb_packet_free(&pkt);
	fb_decoder_free(&decoder);

	*out_duration = duration;
	return 0;
}

int fb_probe_read_subtitle_stream(const char *filename, int stream_index,
								  FBSubtitleCallback callback,
								  void *userdata)
{
	if (!filename || !callback) {
		return AVERROR(EINVAL);
	}

	FBDecoder *decoder = fb_decoder_create();
	if (fb_decoder_open(decoder, filename, stream_index) < 0) {
		fb_decoder_free(&decoder);
		return AVERROR_EXTERNAL;
	}

	// Limit to SRT for now (mirrors the editor's historical behavior)
	FBStreamInfo stream_info;
	if (fb_decoder_get_stream_info(decoder, &stream_info) < 0 ||
		stream_info.codec_id != (int)AV_CODEC_ID_SUBRIP) {
		fb_decoder_free(&decoder);
		return AVERROR(EINVAL);
	}

	FBPacket *pkt = fb_packet_alloc();

	while (fb_decoder_get_packet(decoder, pkt) >= 0) {
		callback(fb_packet_get_pts(pkt), fb_packet_get_duration(pkt),
				 reinterpret_cast<const char *>(fb_packet_get_data(pkt)),
				 fb_packet_get_size(pkt), userdata);
	}

	fb_packet_free(&pkt);
	fb_decoder_free(&decoder);

	return 0;
}
