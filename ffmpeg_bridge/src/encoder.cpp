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

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <string>
#include <utility>
#include <vector>

namespace
{

AVPixelFormat ConvertJPEGSpaceToRegularSpace(AVPixelFormat f)
{
	switch (f) {
	case AV_PIX_FMT_YUVJ420P:
		return AV_PIX_FMT_YUV420P;
	case AV_PIX_FMT_YUVJ422P:
		return AV_PIX_FMT_YUV422P;
	case AV_PIX_FMT_YUVJ444P:
		return AV_PIX_FMT_YUV444P;
	case AV_PIX_FMT_YUVJ440P:
		return AV_PIX_FMT_YUV440P;
	case AV_PIX_FMT_YUVJ411P:
		return AV_PIX_FMT_YUV411P;
	default:
		break;
	}

	return f;
}

const AVCodec *FindEncoder(int codec, int sample_format)
{
	switch (codec) {
	case FB_CODEC_H264:
		return avcodec_find_encoder_by_name("libx264");
	case FB_CODEC_H264RGB:
		return avcodec_find_encoder_by_name("libx264rgb");
	case FB_CODEC_DNXHD:
		return avcodec_find_encoder(AV_CODEC_ID_DNXHD);
	case FB_CODEC_PRORES:
		return avcodec_find_encoder(AV_CODEC_ID_PRORES);
	case FB_CODEC_CINEFORM:
		return avcodec_find_encoder(AV_CODEC_ID_CFHD);
	case FB_CODEC_H265:
		return avcodec_find_encoder(AV_CODEC_ID_HEVC);
	case FB_CODEC_VP9:
		return avcodec_find_encoder(AV_CODEC_ID_VP9);
	case FB_CODEC_AV1: {
		const AVCodec *encoder = avcodec_find_encoder_by_name("libsvtav1");
		if (!encoder) {
			encoder = avcodec_find_encoder(AV_CODEC_ID_AV1);
		}
		return encoder;
	}
	case FB_CODEC_OPENEXR:
		return avcodec_find_encoder(AV_CODEC_ID_EXR);
	case FB_CODEC_PNG:
		return avcodec_find_encoder(AV_CODEC_ID_PNG);
	case FB_CODEC_TIFF:
		return avcodec_find_encoder(AV_CODEC_ID_TIFF);
	case FB_CODEC_MP2:
		return avcodec_find_encoder(AV_CODEC_ID_MP2);
	case FB_CODEC_MP3:
		return avcodec_find_encoder(AV_CODEC_ID_MP3);
	case FB_CODEC_AAC:
		return avcodec_find_encoder(AV_CODEC_ID_AAC);
	case FB_CODEC_PCM:
		switch (sample_format) {
		case FB_SAMPLE_FMT_U8:
			return avcodec_find_encoder(AV_CODEC_ID_PCM_U8);
		case FB_SAMPLE_FMT_S16:
			return avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
		case FB_SAMPLE_FMT_S32:
			return avcodec_find_encoder(AV_CODEC_ID_PCM_S32LE);
		case FB_SAMPLE_FMT_S64:
			return avcodec_find_encoder(AV_CODEC_ID_PCM_S64LE);
		case FB_SAMPLE_FMT_FLT:
			return avcodec_find_encoder(AV_CODEC_ID_PCM_F32LE);
		case FB_SAMPLE_FMT_DBL:
			return avcodec_find_encoder(AV_CODEC_ID_PCM_F64LE);
		default:
			break;
		}
		break;
	case FB_CODEC_FLAC:
		return avcodec_find_encoder(AV_CODEC_ID_FLAC);
	case FB_CODEC_OPUS:
		return avcodec_find_encoder(AV_CODEC_ID_OPUS);
	case FB_CODEC_VORBIS:
		return avcodec_find_encoder(AV_CODEC_ID_VORBIS);
	case FB_CODEC_SRT:
		return avcodec_find_encoder(AV_CODEC_ID_SUBRIP);
	default:
		break;
	}

	return nullptr;
}

} // namespace

struct FBEncoder {
	// Deep-copied configuration
	std::string filename;

	int video_enabled = 0;
	int video_codec = FB_CODEC_NONE;
	int video_width = 0;
	int video_height = 0;
	int video_pixel_aspect_num = 1;
	int video_pixel_aspect_den = 1;
	int video_time_base_num = 0;
	int video_time_base_den = 1;
	int video_frame_rate_num = 0;
	int video_frame_rate_den = 1;
	std::string video_pix_fmt;
	// In AVPixelFormat space (translated from the FB config value at create)
	int video_src_pix_fmt = FB_PIX_FMT_NONE;
	int video_color_range = FB_COLOR_RANGE_UNSPEC;
	int video_field_order = FB_FIELD_ORDER_PROGRESSIVE;
	int64_t video_bit_rate = 0;
	int64_t video_min_bit_rate = 0;
	int64_t video_max_bit_rate = 0;
	int64_t video_buffer_size = 0;
	int video_threads = 0;
	int video_color_srgb = 0;
	std::vector<std::pair<std::string, std::string>> video_opts;

	int audio_enabled = 0;
	int audio_codec = FB_CODEC_NONE;
	int audio_sample_rate = 0;
	uint64_t audio_channel_layout_mask = 0;
	int audio_sample_format = FB_SAMPLE_FMT_NONE;
	int64_t audio_bit_rate = 0;

	int subtitles_enabled = 0;
	int subtitle_codec = FB_CODEC_NONE;
	std::vector<uint8_t> subtitle_header;

	// Runtime state
	AVFormatContext *fmt_ctx = nullptr;

	AVStream *video_stream = nullptr;
	AVCodecContext *video_codec_ctx = nullptr;
	AVFilterGraph *video_scale_ctx = nullptr;
	AVFilterContext *video_buffersrc_ctx = nullptr;
	AVFilterContext *video_buffersink_ctx = nullptr;

	AVStream *audio_stream = nullptr;
	AVCodecContext *audio_codec_ctx = nullptr;
	SwrContext *audio_resample_ctx = nullptr;
	AVFrame *audio_frame = nullptr;
	int audio_max_samples = 0;
	int audio_frame_offset = 0;
	int64_t audio_write_count = 0;

	AVStream *subtitle_stream = nullptr;
	AVCodecContext *subtitle_codec_ctx = nullptr;

	bool open = false;

	char error[1024] = { 0 };

	void SetError(const char *context, int error_code)
	{
		fb::SetError(error, sizeof(error), context, error_code);
	}

	void SetError(const char *message)
	{
		snprintf(error, sizeof(error), "%s", message);
	}

	bool WriteAVFrame(AVFrame *frame, AVCodecContext *codec_ctx,
					  AVStream *stream);
	bool InitializeStream(AVMediaType type, AVStream **stream,
						  AVCodecContext **codec_ctx, int codec);
	bool InitializeCodecContext(AVStream **stream, AVCodecContext **codec_ctx,
								const AVCodec *codec);
	bool SetupCodecContext(AVStream *stream, AVCodecContext *codec_ctx,
						   const AVCodec *codec);
	void FlushEncoders();
	void FlushCodecCtx(AVCodecContext *codec_ctx, AVStream *stream);
	bool InitializeResampleContext(int sample_format, int sample_rate,
								   uint64_t channel_layout_mask);
	bool WriteAudioData(int sample_format, int sample_rate,
						uint64_t channel_layout_mask, const uint8_t **input_data,
						int input_sample_count);
};

FBEncoder *fb_encoder_create(const FBEncoderConfig *config)
{
	if (!config || !config->filename) {
		return nullptr;
	}

	FBEncoder *e = new FBEncoder;

	e->filename = config->filename;

	e->video_enabled = config->video_enabled;
	e->video_codec = config->video_codec;
	e->video_width = config->video_width;
	e->video_height = config->video_height;
	e->video_pixel_aspect_num = config->video_pixel_aspect_num;
	e->video_pixel_aspect_den = config->video_pixel_aspect_den;
	e->video_time_base_num = config->video_time_base_num;
	e->video_time_base_den = config->video_time_base_den;
	e->video_frame_rate_num = config->video_frame_rate_num;
	e->video_frame_rate_den = config->video_frame_rate_den;
	if (config->video_pix_fmt) {
		e->video_pix_fmt = config->video_pix_fmt;
	}
	// Stored in AVPixelFormat space; the public config value is an
	// FB_PIX_FMT_* identifier.
	e->video_src_pix_fmt = fb::PixFmtToAV(config->video_src_pix_fmt);
	e->video_color_range = config->video_color_range;
	e->video_field_order = config->video_field_order;
	e->video_bit_rate = config->video_bit_rate;
	e->video_min_bit_rate = config->video_min_bit_rate;
	e->video_max_bit_rate = config->video_max_bit_rate;
	e->video_buffer_size = config->video_buffer_size;
	e->video_threads = config->video_threads;
	e->video_color_srgb = config->video_color_srgb;
	for (int i = 0; i < config->video_opt_count; i++) {
		if (config->video_opt_keys[i] && config->video_opt_values[i]) {
			e->video_opts.emplace_back(config->video_opt_keys[i],
									   config->video_opt_values[i]);
		}
	}

	e->audio_enabled = config->audio_enabled;
	e->audio_codec = config->audio_codec;
	e->audio_sample_rate = config->audio_sample_rate;
	e->audio_channel_layout_mask = config->audio_channel_layout_mask;
	e->audio_sample_format = config->audio_sample_format;
	e->audio_bit_rate = config->audio_bit_rate;

	e->subtitles_enabled = config->subtitles_enabled;
	e->subtitle_codec = config->subtitle_codec;
	if (config->subtitle_header && config->subtitle_header_size > 0) {
		e->subtitle_header.assign(config->subtitle_header,
								  config->subtitle_header +
									  config->subtitle_header_size);
	}

	return e;
}

void fb_encoder_free(FBEncoder **encoder)
{
	if (encoder && *encoder) {
		fb_encoder_close(*encoder);
		delete *encoder;
		*encoder = nullptr;
	}
}

int fb_encoder_open(FBEncoder *e)
{
	if (!e) {
		return AVERROR(EINVAL);
	}

	if (e->open) {
		return 0;
	}

	int error_code;

	// Create output format context
	error_code = avformat_alloc_output_context2(&e->fmt_ctx, nullptr, nullptr,
												e->filename.c_str());
	if (error_code < 0) {
		e->SetError("Failed to allocate output context", error_code);
		return error_code;
	}

	// Initialize a video stream if it's enabled
	if (e->video_enabled) {
		if (!e->InitializeStream(AVMEDIA_TYPE_VIDEO, &e->video_stream,
								 &e->video_codec_ctx, e->video_codec)) {
			return AVERROR_EXTERNAL;
		}

		// This is the pixel format the encoder wants to encode to
		AVPixelFormat encoder_pix_fmt = e->video_codec_ctx->pix_fmt;

		e->video_scale_ctx = avfilter_graph_alloc();
		if (!e->video_scale_ctx) {
			e->SetError("Failed to allocate filter graph");
			return AVERROR_EXTERNAL;
		}

		static const int FILTER_ARG_SZ = 1024;
		char filter_args[FILTER_ARG_SZ];

		snprintf(filter_args, FILTER_ARG_SZ,
				 "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
				 e->video_width, e->video_height, e->video_src_pix_fmt,
				 e->video_time_base_num, e->video_time_base_den,
				 e->video_pixel_aspect_num, e->video_pixel_aspect_den);

		avfilter_graph_create_filter(&e->video_buffersrc_ctx,
									 avfilter_get_by_name("buffer"), "in",
									 filter_args, nullptr, e->video_scale_ctx);
		avfilter_graph_create_filter(&e->video_buffersink_ctx,
									 avfilter_get_by_name("buffersink"), "out",
									 nullptr, nullptr, e->video_scale_ctx);

		AVFilterContext *last_filter = e->video_buffersrc_ctx;

		{
			// Set color range
			AVFilterContext *range_filter;

			snprintf(filter_args, FILTER_ARG_SZ, "in_range=full:out_range=%s",
					 e->video_color_range == FB_COLOR_RANGE_JPEG ? "full" :
																   "limited");

			avfilter_graph_create_filter(&range_filter,
										 avfilter_get_by_name("scale"), "range",
										 filter_args, nullptr,
										 e->video_scale_ctx);

			avfilter_link(last_filter, 0, range_filter, 0);
			last_filter = range_filter;
		}

		if (e->video_src_pix_fmt != encoder_pix_fmt) {
			// Transform pixel format
			AVFilterContext *format_filter;

			snprintf(filter_args, FILTER_ARG_SZ, "pix_fmts=%u", encoder_pix_fmt);

			avfilter_graph_create_filter(&format_filter,
										 avfilter_get_by_name("format"),
										 "format", filter_args, nullptr,
										 e->video_scale_ctx);

			avfilter_link(last_filter, 0, format_filter, 0);
			last_filter = format_filter;
		}

		avfilter_link(last_filter, 0, e->video_buffersink_ctx, 0);

		if (avfilter_graph_config(e->video_scale_ctx, nullptr) < 0) {
			e->SetError("Failed to configure filter graph");
			return AVERROR_EXTERNAL;
		}
	}

	// Initialize an audio stream if it's enabled
	if (e->audio_enabled) {
		if (!e->InitializeStream(AVMEDIA_TYPE_AUDIO, &e->audio_stream,
								 &e->audio_codec_ctx, e->audio_codec)) {
			return AVERROR_EXTERNAL;
		}
	}

	// Initialize a subtitle stream if it's enabled
	if (e->subtitles_enabled) {
		if (!e->InitializeStream(AVMEDIA_TYPE_SUBTITLE, &e->subtitle_stream,
								 &e->subtitle_codec_ctx, e->subtitle_codec)) {
			return AVERROR_EXTERNAL;
		}
	}

	av_dump_format(e->fmt_ctx, 0, e->filename.c_str(), 1);

	// Open output file for writing
	error_code = avio_open(&e->fmt_ctx->pb, e->filename.c_str(), AVIO_FLAG_WRITE);
	if (error_code < 0) {
		e->SetError("Failed to open IO context", error_code);
		return error_code;
	}

	// Write header
	error_code = avformat_write_header(e->fmt_ctx, nullptr);
	if (error_code < 0) {
		e->SetError("Failed to write format header", error_code);
		return error_code;
	}

	e->open = true;
	return 0;
}

int fb_encoder_write_video_frame(FBEncoder *e, int width, int height,
								 int pix_fmt, const uint8_t *data, int linesize,
								 double time_seconds)
{
	if (!e || !e->open || !data) {
		return AVERROR(EINVAL);
	}

	// Use the filter graph to convert formats/linesizes
	AVFrame *input_frame = av_frame_alloc();
	if (!input_frame) {
		e->SetError("Failed to allocate input frame");
		return AVERROR(ENOMEM);
	}

	input_frame->width = width;
	input_frame->height = height;
	input_frame->format = fb::PixFmtToAV(pix_fmt);
	input_frame->data[0] = const_cast<uint8_t *>(data);
	input_frame->linesize[0] = linesize;

	input_frame->color_primaries = e->video_codec_ctx->color_primaries;
	input_frame->color_trc = e->video_codec_ctx->color_trc;
	input_frame->colorspace = e->video_codec_ctx->colorspace;
	input_frame->color_range = e->video_codec_ctx->color_range;

	int r = av_buffersrc_add_frame_flags(e->video_buffersrc_ctx, input_frame,
										 AV_BUFFERSRC_FLAG_KEEP_REF);
	av_frame_free(&input_frame);
	if (r < 0) {
		e->SetError("Failed to add frame to filter graph", r);
		return r;
	}

	AVFrame *encoded_frame = av_frame_alloc();
	if (!encoded_frame) {
		e->SetError("Failed to allocate encode frame");
		return AVERROR(ENOMEM);
	}

	r = av_buffersink_get_frame(e->video_buffersink_ctx, encoded_frame);
	if (r < 0) {
		av_frame_free(&encoded_frame);
		e->SetError("Failed to retrieve frame from buffer sink", r);
		return r;
	}

	encoded_frame->pts =
		llround(time_seconds / av_q2d(e->video_codec_ctx->time_base));

	bool result =
		e->WriteAVFrame(encoded_frame, e->video_codec_ctx, e->video_stream);

	av_frame_free(&encoded_frame);

	return result ? 0 : AVERROR_EXTERNAL;
}

bool FBEncoder::WriteAudioData(int sample_format, int sample_rate,
							   uint64_t channel_layout_mask,
							   const uint8_t **input_data,
							   int input_sample_count)
{
	if (!InitializeResampleContext(sample_format, sample_rate,
								   channel_layout_mask)) {
		SetError("Failed to initialize resample context");
		return false;
	}

	bool result = true;

	// Create output buffer
	int output_sample_count =
		input_sample_count ?
			swr_get_out_samples(audio_resample_ctx, input_sample_count) :
			102400;
	uint8_t **output_data = nullptr;
	int output_linesize;
	av_samples_alloc_array_and_samples(
		&output_data, &output_linesize,
		audio_stream->codecpar->ch_layout.nb_channels, output_sample_count,
		static_cast<AVSampleFormat>(audio_stream->codecpar->format), 0);

	// Perform conversion
	int converted = swr_convert(audio_resample_ctx, output_data,
								output_sample_count, input_data,
								input_sample_count);
	if (converted > 0) {
		// Split sample buffer into frames
		for (int i = 0; i < converted;) {
			int frame_remaining_samples = audio_max_samples - audio_frame_offset;
			int converted_remaining_samples = converted - i;

			int copy_length =
				frame_remaining_samples < converted_remaining_samples ?
					frame_remaining_samples :
					converted_remaining_samples;

			av_samples_copy(audio_frame->data, output_data, audio_frame_offset,
							i, copy_length, audio_frame->ch_layout.nb_channels,
							static_cast<AVSampleFormat>(audio_frame->format));

			audio_frame_offset += copy_length;
			i += copy_length;

			if (audio_frame_offset == audio_max_samples ||
				(i == converted && !input_data)) {
				// Got all the samples we needed, write the frame
				audio_frame->pts = av_rescale_q(
					audio_write_count, { 1, audio_codec_ctx->sample_rate },
					audio_codec_ctx->time_base);

				WriteAVFrame(audio_frame, audio_codec_ctx, audio_stream);
				audio_write_count += audio_frame_offset;
				audio_frame_offset = 0;
			}
		}
	} else if (converted < 0) {
		SetError("Failed to resample audio", converted);
		result = false;
	}

	if (!input_data && audio_frame_offset > 0) {
		audio_frame->nb_samples = audio_frame_offset;
		audio_frame->pts =
			av_rescale_q(audio_write_count, { 1, audio_codec_ctx->sample_rate },
						 audio_codec_ctx->time_base);
		WriteAVFrame(audio_frame, audio_codec_ctx, audio_stream);
	}

	// Free buffers created
	if (output_data) {
		av_freep(&output_data[0]);
		av_freep(&output_data);
	}

	return result;
}

int fb_encoder_write_audio(FBEncoder *e, const uint8_t *const *channel_data,
						   int channels, int sample_format, int sample_rate,
						   uint64_t channel_layout_mask, int64_t sample_count)
{
	if (!e || !e->open) {
		return AVERROR(EINVAL);
	}

	if (!channel_data || sample_count == 0) {
		// Nothing to write (matches the historical empty-buffer early-out)
		return 0;
	}

	int bytes_per_sample =
		av_get_bytes_per_sample(static_cast<AVSampleFormat>(sample_format));
	int planar =
		av_sample_fmt_is_planar(static_cast<AVSampleFormat>(sample_format));

	bool result = true;

	size_t start = 0;
	size_t end = size_t(sample_count);
	const size_t max_frame = 48000;

	while (result && start < end) {
		// Create input buffer
		uint8_t **input_data = nullptr;
		size_t input_sample_count =
			(end - start) < max_frame ? (end - start) : max_frame;
		int input_linesize;

		int r = av_samples_alloc_array_and_samples(
			&input_data, &input_linesize, channels, int(input_sample_count),
			static_cast<AVSampleFormat>(sample_format), 0);

		if (r < 0) {
			e->SetError("Failed to allocate sample array", r);
			return r;
		} else {
			if (planar) {
				for (int i = 0; i < channels; i++) {
					memcpy(input_data[i],
						   channel_data[i] + start * size_t(bytes_per_sample),
						   input_sample_count * size_t(bytes_per_sample));
				}
			} else {
				size_t stride = size_t(bytes_per_sample) * size_t(channels);
				memcpy(input_data[0], channel_data[0] + start * stride,
					   input_sample_count * stride);
			}

			start += input_sample_count;
		}

		result = e->WriteAudioData(sample_format, sample_rate,
								   channel_layout_mask,
								   const_cast<const uint8_t **>(input_data),
								   int(input_sample_count));

		if (input_data) {
			av_freep(&input_data[0]);
			av_freep(&input_data);
		}
	}

	return result ? 0 : AVERROR_EXTERNAL;
}

int fb_encoder_write_subtitle(FBEncoder *e, const char *utf8_text,
							  double in_seconds, double duration_seconds)
{
	if (!e || !e->open || !utf8_text) {
		return AVERROR(EINVAL);
	}

	AVPacket *pkt = av_packet_alloc();
	if (!pkt) {
		return AVERROR(ENOMEM);
	}

	pkt->stream_index = e->subtitle_stream->index;
	pkt->data = reinterpret_cast<uint8_t *>(const_cast<char *>(utf8_text));
	pkt->size = int(strlen(utf8_text));

	// Convert seconds to the codec timebase, rounding down
	double d = in_seconds / av_q2d(e->subtitle_codec_ctx->time_base);
	const double eps = 0.000000000001;
	int64_t pts;
	if (d > ceil(d) - eps) {
		pts = int64_t(ceil(d));
	} else {
		pts = int64_t(floor(d));
	}
	pkt->pts = pts;

	pkt->duration = av_rescale_q(llround(duration_seconds * 1000), { 1, 1000 },
								 e->subtitle_codec_ctx->time_base);
	pkt->dts = pkt->pts;
	av_packet_rescale_ts(pkt, e->subtitle_codec_ctx->time_base,
						 e->subtitle_stream->time_base);

	int err = av_interleaved_write_frame(e->fmt_ctx, pkt);
	bool ret = true;

	if (err < 0) {
		e->SetError("Failed to write interleaved packet", err);
		ret = false;
	}

	av_packet_free(&pkt);

	return ret ? 0 : err;
}

void fb_encoder_close(FBEncoder *e)
{
	if (!e) {
		return;
	}

	if (e->open) {
		// Flush encoders
		e->FlushEncoders();

		// We've written a header, so we'll write a trailer
		av_write_trailer(e->fmt_ctx);
		avio_closep(&e->fmt_ctx->pb);

		e->open = false;
	}

	if (e->audio_resample_ctx) {
		swr_free(&e->audio_resample_ctx);
		e->audio_resample_ctx = nullptr;
	}

	if (e->audio_frame) {
		av_frame_free(&e->audio_frame);
		e->audio_frame = nullptr;
	}

	if (e->video_scale_ctx) {
		avfilter_graph_free(&e->video_scale_ctx);
		e->video_scale_ctx = nullptr;
		e->video_buffersrc_ctx = nullptr;
		e->video_buffersink_ctx = nullptr;
	}

	if (e->video_codec_ctx) {
		avcodec_free_context(&e->video_codec_ctx);
		e->video_codec_ctx = nullptr;
	}

	if (e->audio_codec_ctx) {
		avcodec_free_context(&e->audio_codec_ctx);
		e->audio_codec_ctx = nullptr;
	}

	if (e->subtitle_codec_ctx) {
		avcodec_free_context(&e->subtitle_codec_ctx);
		e->subtitle_codec_ctx = nullptr;
	}

	if (e->fmt_ctx) {
		// NOTE: This also frees the streams
		avformat_free_context(e->fmt_ctx);
		e->fmt_ctx = nullptr;
		e->video_stream = nullptr;
		e->audio_stream = nullptr;
		e->subtitle_stream = nullptr;
	}
}

const char *fb_encoder_get_error(const FBEncoder *encoder)
{
	return encoder ? encoder->error : "";
}

bool FBEncoder::WriteAVFrame(AVFrame *frame, AVCodecContext *codec_ctx,
							 AVStream *stream)
{
	// Send raw frame to the encoder
	int error_code = avcodec_send_frame(codec_ctx, frame);
	if (error_code < 0) {
		SetError("Failed to send frame to encoder", error_code);
		return false;
	}

	bool succeeded = false;

	AVPacket *pkt = av_packet_alloc();

	// Retrieve packets from encoder
	while (error_code >= 0) {
		error_code = avcodec_receive_packet(codec_ctx, pkt);

		// EAGAIN just means the encoder wants another frame before encoding
		if (error_code == AVERROR(EAGAIN)) {
			break;
		} else if (error_code < 0) {
			SetError("Failed to receive packet from decoder", error_code);
			goto fail;
		}

		// Set packet stream index
		pkt->stream_index = stream->index;

		av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);

		// Write packet to file
		error_code = av_interleaved_write_frame(fmt_ctx, pkt);
		if (error_code < 0) {
			SetError("Failed to write interleaved packet", error_code);
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

bool FBEncoder::InitializeStream(AVMediaType type, AVStream **stream_ptr,
								 AVCodecContext **codec_ctx_ptr, int codec)
{
	if (type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO &&
		type != AVMEDIA_TYPE_SUBTITLE) {
		SetError("Cannot initialize a stream that is not a video, audio, or subtitle type");
		return false;
	}

	// Find encoder
	const AVCodec *encoder = FindEncoder(codec, audio_sample_format);
	if (!encoder) {
		char msg[128];
		snprintf(msg, sizeof(msg), "Failed to find codec for 0x%x", codec);
		SetError(msg);
		return false;
	}

	if (encoder->type != type) {
		SetError("Retrieved unexpected codec type for codec");
		return false;
	}

	if (!InitializeCodecContext(stream_ptr, codec_ctx_ptr, encoder)) {
		return false;
	}

	// Set codec parameters
	AVCodecContext *codec_ctx = *codec_ctx_ptr;
	AVStream *stream = *stream_ptr;

	if (type == AVMEDIA_TYPE_VIDEO) {
		codec_ctx->width = video_width;
		codec_ctx->height = video_height;
		codec_ctx->sample_aspect_ratio = { video_pixel_aspect_num,
										   video_pixel_aspect_den };
		codec_ctx->time_base = { video_time_base_num, video_time_base_den };
		codec_ctx->framerate = { video_frame_rate_num, video_frame_rate_den };
		codec_ctx->pix_fmt = av_get_pix_fmt(video_pix_fmt.c_str());
		codec_ctx->color_range = video_color_range == FB_COLOR_RANGE_JPEG ?
									 AVCOL_RANGE_JPEG :
									 AVCOL_RANGE_MPEG;

		if (video_field_order != FB_FIELD_ORDER_PROGRESSIVE) {
			// FIXME: I actually don't know what these flags do, the documentation helpfully doesn't
			//        explain them at all. I hope using both of them is the right thing to do.
			codec_ctx->flags |= AV_CODEC_FLAG_INTERLACED_DCT |
								AV_CODEC_FLAG_INTERLACED_ME;

			if (video_field_order == FB_FIELD_ORDER_TT) {
				codec_ctx->field_order = AV_FIELD_TT;
			} else {
				codec_ctx->field_order = AV_FIELD_BB;

				if (video_codec == FB_CODEC_H264 ||
					video_codec == FB_CODEC_H264RGB) {
					// For some reason, FFmpeg doesn't set libx264's bff flag so we have to do it ourselves
					av_opt_set(codec_ctx->priv_data, "x264opts", "bff=1",
							   AV_OPT_SEARCH_CHILDREN);
				}
			}
		}

		// Set custom options
		for (const auto &opt : video_opts) {
			av_opt_set(codec_ctx->priv_data, opt.first.c_str(),
					   opt.second.c_str(), AV_OPT_SEARCH_CHILDREN);
		}

		if (video_bit_rate > 0) {
			codec_ctx->bit_rate = video_bit_rate;
		}

		if (video_min_bit_rate > 0) {
			codec_ctx->rc_min_rate = video_min_bit_rate;
		}

		if (video_max_bit_rate > 0) {
			codec_ctx->rc_max_rate = video_max_bit_rate;
		}

		if (video_buffer_size > 0) {
			codec_ctx->rc_buffer_size = static_cast<int>(video_buffer_size);
		}

		// nclc tags. See https://ffmpeg.org/doxygen/4.0/pixfmt_8h.html#ad384ee5a840bafd73daef08e6d9cafe7
		if (video_color_srgb) {
			codec_ctx->color_primaries = AVCOL_PRI_BT709;
			codec_ctx->color_trc = AVCOL_TRC_IEC61966_2_1;
			codec_ctx->colorspace = AVCOL_SPC_BT709;
		} else { // Assume Rec.709
			codec_ctx->color_primaries = AVCOL_PRI_BT709;
			codec_ctx->color_trc = AVCOL_TRC_BT709;
			codec_ctx->colorspace = AVCOL_SPC_BT709;
		}

	} else if (type == AVMEDIA_TYPE_AUDIO) {
		codec_ctx->sample_rate = audio_sample_rate;
		av_channel_layout_from_mask(&codec_ctx->ch_layout,
									audio_channel_layout_mask);
		codec_ctx->sample_fmt =
			static_cast<AVSampleFormat>(audio_sample_format);
		codec_ctx->time_base = { 1, codec_ctx->sample_rate };

		if (audio_bit_rate > 0) {
			codec_ctx->bit_rate = audio_bit_rate;
		}

	} else if (type == AVMEDIA_TYPE_SUBTITLE) {
		codec_ctx->time_base = av_get_time_base_q();

		if (!subtitle_header.empty()) {
			codec_ctx->subtitle_header =
				new uint8_t[subtitle_header.size()];
			memcpy(codec_ctx->subtitle_header, subtitle_header.data(),
				   subtitle_header.size());
			codec_ctx->subtitle_header_size = int(subtitle_header.size());
		}
	}

	if (!SetupCodecContext(stream, codec_ctx, encoder)) {
		return false;
	}

	return true;
}

bool FBEncoder::InitializeCodecContext(AVStream **stream,
									   AVCodecContext **codec_ctx,
									   const AVCodec *codec)
{
	*stream = avformat_new_stream(fmt_ctx, nullptr);
	if (!(*stream)) {
		SetError("Failed to allocate AVStream");
		return false;
	}

	// Allocate a codec context
	*codec_ctx = avcodec_alloc_context3(codec);
	if (!(*codec_ctx)) {
		SetError("Failed to allocate AVCodecContext");
		return false;
	}

	return true;
}

bool FBEncoder::SetupCodecContext(AVStream *stream, AVCodecContext *codec_ctx,
								  const AVCodec *codec)
{
	int error_code;

	if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
		codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	AVDictionary *codec_opts = nullptr;

	// Set thread count
	if (video_threads == 0) {
		av_dict_set(&codec_opts, "threads", "auto", 0);
	} else {
		char thread_val[16];
		snprintf(thread_val, sizeof(thread_val), "%d", video_threads);
		av_dict_set(&codec_opts, "threads", thread_val, 0);
	}

	// Try to open encoder
	error_code = avcodec_open2(codec_ctx, codec, &codec_opts);
	av_dict_free(&codec_opts);
	if (error_code < 0) {
		SetError("Failed to open encoder", error_code);
		return false;
	}

	// Copy context settings to codecpar object
	error_code = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
	if (error_code < 0) {
		SetError("Failed to copy codec parameters to stream", error_code);
		return false;
	}

	if (codec->type == AVMEDIA_TYPE_VIDEO) {
		stream->avg_frame_rate = codec_ctx->framerate;
	}

	return true;
}

void FBEncoder::FlushEncoders()
{
	if (video_codec_ctx) {
		FlushCodecCtx(video_codec_ctx, video_stream);
	}

	if (audio_codec_ctx) {
		FlushCodecCtx(audio_codec_ctx, audio_stream);
	}

	if (fmt_ctx) {
		if (fmt_ctx->oformat->flags) {
			int r = av_interleaved_write_frame(fmt_ctx, nullptr);
			if (r < 0) {
				SetError("Failed to write interleaved packet", r);
			}
		}
	}
}

void FBEncoder::FlushCodecCtx(AVCodecContext *codec_ctx, AVStream *stream)
{
	avcodec_send_frame(codec_ctx, nullptr);
	AVPacket *pkt = av_packet_alloc();

	int error_code;
	do {
		error_code = avcodec_receive_packet(codec_ctx, pkt);

		if (error_code < 0) {
			break;
		}

		pkt->stream_index = stream->index;
		av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);
		int r = av_interleaved_write_frame(fmt_ctx, pkt);
		if (r < 0) {
			SetError("Failed to write interleaved packet", r);
			break;
		}
		av_packet_unref(pkt);
	} while (error_code >= 0);

	av_packet_free(&pkt);
}

bool FBEncoder::InitializeResampleContext(int sample_format, int sample_rate,
										  uint64_t channel_layout_mask)
{
	if (audio_resample_ctx) {
		return true;
	}

	AVChannelLayout layout;
	fb::ChannelLayoutFromMask(&layout, channel_layout_mask, 0);

	// Create resample context
	swr_alloc_set_opts2(&audio_resample_ctx, &audio_codec_ctx->ch_layout,
						audio_codec_ctx->sample_fmt,
						audio_codec_ctx->sample_rate, &layout,
						static_cast<AVSampleFormat>(sample_format), sample_rate,
						0, nullptr);
	av_channel_layout_uninit(&layout);

	if (!audio_resample_ctx) {
		return false;
	}

	int err = swr_init(audio_resample_ctx);
	if (err < 0) {
		SetError("Failed to create resampling context", err);
		return false;
	}

	audio_max_samples = audio_codec_ctx->frame_size;
	if (!audio_max_samples) {
		// If not set, use another frame size
		if (video_enabled) {
			// If we're encoding video, use enough samples to cover roughly one frame of video
			audio_max_samples =
				int(int64_t(audio_sample_rate) * video_time_base_num /
					video_time_base_den);
		} else {
			// If no video, just use an arbitrary number
			audio_max_samples = 256;
		}
	}

	audio_frame = av_frame_alloc();
	if (!audio_frame) {
		return false;
	}

	audio_frame->ch_layout = audio_codec_ctx->ch_layout;
	audio_frame->format = audio_codec_ctx->sample_fmt;
	audio_frame->nb_samples = audio_max_samples;

	err = av_frame_get_buffer(audio_frame, 0);
	if (err < 0) {
		SetError("Failed to create audio frame", err);
		return false;
	}

	audio_frame_offset = 0;
	audio_write_count = 0;

	return true;
}

int fb_encoder_codec_get_pixel_formats(int codec, const char **names,
									   int max_names)
{
	const AVCodec *codec_info = FindEncoder(codec, FB_SAMPLE_FMT_NONE);
	if (!codec_info || !codec_info->pix_fmts) {
		return 0;
	}

	int count = 0;
	for (int i = 0; codec_info->pix_fmts[i] != AV_PIX_FMT_NONE; i++) {
		AVPixelFormat fmt = codec_info->pix_fmts[i];
		if (ConvertJPEGSpaceToRegularSpace(fmt) != fmt) {
			// This is a deprecated "JPEG" space, skip it
			continue;
		}

		if (names && count < max_names) {
			names[count] = av_get_pix_fmt_name(fmt);
		}
		count++;
	}

	return count;
}

int fb_encoder_codec_get_sample_formats(int codec, int *fmts, int max_fmts)
{
	const AVCodec *codec_info = FindEncoder(codec, FB_SAMPLE_FMT_NONE);
	if (!codec_info || !codec_info->sample_fmts) {
		return 0;
	}

	int count = 0;
	for (int i = 0; codec_info->sample_fmts[i] != AV_SAMPLE_FMT_NB; i++) {
		if (fmts && count < max_fmts) {
			fmts[count] = codec_info->sample_fmts[i];
		}
		count++;
	}

	return count;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
