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
#include <stdlib.h>
#include <string.h>

#include <initializer_list>

namespace
{

constexpr int64_t kAnalyzeDurationUs = 5000000;
constexpr int64_t kProbeSizeBytes = 20000000;

void ApplyFormatOpenOptions(AVDictionary **opts)
{
	av_dict_set_int(opts, "analyzeduration", kAnalyzeDurationUs, 0);
	av_dict_set_int(opts, "probesize", kProbeSizeBytes, 0);
}

void TuneFormatContext(AVFormatContext *ctx)
{
	if (!ctx) {
		return;
	}

	ctx->probesize = kProbeSizeBytes;
	ctx->max_analyze_duration = kAnalyzeDurationUs;
}

void DiscardSubtitleStreams(AVFormatContext *ctx)
{
	if (!ctx) {
		return;
	}

	for (unsigned int i = 0; i < ctx->nb_streams; i++) {
		AVStream *stream = ctx->streams[i];
		if (stream && stream->codecpar &&
			stream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) {
			stream->discard = AVDISCARD_ALL;
		}
	}
}

} // namespace

struct FBDecoder {
	AVFormatContext *fmt_ctx = nullptr;
	AVCodecContext *codec_ctx = nullptr;
	AVStream *avstream = nullptr;
	AVDictionary *opts = nullptr;

	AVBufferRef *hw_device_ctx = nullptr;
	AVHWDeviceType hw_device_type = AV_HWDEVICE_TYPE_NONE;
	AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;
	bool hwaccel_enabled = false;

	bool Open(const char *filename, int stream_index);
	void Close();

	static AVHWDeviceType ChooseHardwareDevice();
	static AVPixelFormat GetHardwareFormat(AVCodecContext *ctx,
										   const AVPixelFormat *pix_fmts);
	bool InitHardwareAcceleration(const AVCodec *codec);
	void CleanupHardwareAcceleration();
};

FBDecoder *fb_decoder_create(void)
{
	return new FBDecoder;
}

void fb_decoder_free(FBDecoder **decoder)
{
	if (decoder && *decoder) {
		(*decoder)->Close();
		delete *decoder;
		*decoder = nullptr;
	}
}

bool FBDecoder::Open(const char *filename, int stream_index)
{
	// Open file in a format context
	AVDictionary *format_opts = nullptr;
	ApplyFormatOpenOptions(&format_opts);
	int error_code = avformat_open_input(&fmt_ctx, filename, nullptr, &format_opts);
	av_dict_free(&format_opts);
	TuneFormatContext(fmt_ctx);
	DiscardSubtitleStreams(fmt_ctx);

	if (error_code != 0) {
		fprintf(stderr, "ffmpeg_bridge: failed to open input %s (%d)\n", filename,
				error_code);
		return false;
	}

	// Get stream information from format
	error_code = avformat_find_stream_info(fmt_ctx, nullptr);
	if (error_code < 0) {
		fprintf(stderr, "ffmpeg_bridge: failed to find stream info (%d)\n",
				error_code);
		return false;
	}

	// Get reference to correct AVStream
	avstream = fmt_ctx->streams[stream_index];

	// Find decoder
	const AVCodec *codec = avcodec_find_decoder(avstream->codecpar->codec_id);
	if (codec == nullptr) {
		fprintf(stderr, "ffmpeg_bridge: no decoder for codec %d\n",
				avstream->codecpar->codec_id);
		return false;
	}

	// Allocate context for the decoder
	codec_ctx = avcodec_alloc_context3(codec);
	if (codec_ctx == nullptr) {
		fprintf(stderr, "ffmpeg_bridge: failed to allocate codec context\n");
		return false;
	}

	// Copy parameters from the AVStream to the AVCodecContext
	error_code = avcodec_parameters_to_context(codec_ctx, avstream->codecpar);
	if (error_code < 0) {
		fprintf(stderr, "ffmpeg_bridge: failed to copy codec parameters\n");
		return false;
	}

	// Set multithreading setting
	error_code = av_dict_set(&opts, "threads", "auto", 0);
	if (error_code < 0) {
		fprintf(stderr,
				"ffmpeg_bridge: failed to set codec options, performance may suffer\n");
	}

	// Attempt hardware accelerated decoding first, then fall back to software.
	if (InitHardwareAcceleration(codec)) {
		error_code = avcodec_open2(codec_ctx, codec, &opts);
		if (error_code == 0) {
			hwaccel_enabled = true;
			return true;
		}

		fprintf(stderr,
				"ffmpeg_bridge: failed to open hardware codec, falling back to software (%d)\n",
				error_code);

		// Free the failed context and recreate it for software decoding.
		avcodec_free_context(&codec_ctx);
		CleanupHardwareAcceleration();

		codec_ctx = avcodec_alloc_context3(codec);
		if (codec_ctx == nullptr) {
			fprintf(stderr,
					"ffmpeg_bridge: failed to allocate codec context for software fallback\n");
			return false;
		}

		error_code = avcodec_parameters_to_context(codec_ctx, avstream->codecpar);
		if (error_code < 0) {
			fprintf(stderr, "ffmpeg_bridge: failed to copy codec parameters\n");
			return false;
		}
	}

	// Open codec (software path, or if hardware was not available)
	error_code = avcodec_open2(codec_ctx, codec, &opts);
	if (error_code < 0) {
		fprintf(stderr, "ffmpeg_bridge: failed to open codec %d (%d)\n", codec->id,
				error_code);
		return false;
	}

	return true;
}

AVHWDeviceType FBDecoder::ChooseHardwareDevice()
{
	if (getenv("OAK_DISABLE_HWACCEL") != nullptr) {
		return AV_HWDEVICE_TYPE_NONE;
	}

#if defined(__linux__)
	// Prefer NVIDIA's NVDEC where available, then VAAPI/VDPAU.
	for (AVHWDeviceType type :
		 { AV_HWDEVICE_TYPE_CUDA, AV_HWDEVICE_TYPE_VAAPI, AV_HWDEVICE_TYPE_VDPAU }) {
		if (av_hwdevice_find_type_by_name(av_hwdevice_get_type_name(type)) !=
			AV_HWDEVICE_TYPE_NONE) {
			return type;
		}
	}
#elif defined(_WIN32)
	for (AVHWDeviceType type :
		 { AV_HWDEVICE_TYPE_D3D11VA, AV_HWDEVICE_TYPE_DXVA2, AV_HWDEVICE_TYPE_CUDA }) {
		if (av_hwdevice_find_type_by_name(av_hwdevice_get_type_name(type)) !=
			AV_HWDEVICE_TYPE_NONE) {
			return type;
		}
	}
#elif defined(__APPLE__)
	if (av_hwdevice_find_type_by_name(
			av_hwdevice_get_type_name(AV_HWDEVICE_TYPE_VIDEOTOOLBOX)) !=
		AV_HWDEVICE_TYPE_NONE) {
		return AV_HWDEVICE_TYPE_VIDEOTOOLBOX;
	}
#endif
	return AV_HWDEVICE_TYPE_NONE;
}

AVPixelFormat FBDecoder::GetHardwareFormat(AVCodecContext *ctx,
										   const AVPixelFormat *pix_fmts)
{
	const FBDecoder *inst = static_cast<const FBDecoder *>(ctx->opaque);
	for (const AVPixelFormat *p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
		if (*p == inst->hw_pix_fmt) {
			return *p;
		}
	}

	fprintf(stderr,
			"ffmpeg_bridge: hardware pixel format not supported by decoder, using first software format\n");
	return pix_fmts[0];
}

bool FBDecoder::InitHardwareAcceleration(const AVCodec *codec)
{
	const AVHWDeviceType device_type = ChooseHardwareDevice();
	if (device_type == AV_HWDEVICE_TYPE_NONE) {
		return false;
	}

	// Find the pixel format associated with this device type for this codec.
	hw_pix_fmt = AV_PIX_FMT_NONE;
	for (int i = 0;; i++) {
		const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
		if (!config) {
			break;
		}
		if ((config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
			config->device_type == device_type) {
			hw_pix_fmt = config->pix_fmt;
			break;
		}
	}

	if (hw_pix_fmt == AV_PIX_FMT_NONE) {
		return false;
	}

	hw_device_type = device_type;

	int ret = av_hwdevice_ctx_create(&hw_device_ctx, device_type, nullptr, nullptr,
									 0);
	if (ret < 0) {
		fprintf(stderr,
				"ffmpeg_bridge: failed to create hardware device context (%d)\n",
				ret);
		CleanupHardwareAcceleration();
		return false;
	}

	codec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
	codec_ctx->opaque = this;
	codec_ctx->get_format = GetHardwareFormat;
	// Most hardware decoders do not support frame threading.
	av_dict_set(&opts, "threads", "1", 0);

	return true;
}

void FBDecoder::CleanupHardwareAcceleration()
{
	hwaccel_enabled = false;
	hw_device_type = AV_HWDEVICE_TYPE_NONE;
	hw_pix_fmt = AV_PIX_FMT_NONE;

	if (hw_device_ctx) {
		av_buffer_unref(&hw_device_ctx);
		hw_device_ctx = nullptr;
	}
}

void FBDecoder::Close()
{
	if (opts) {
		av_dict_free(&opts);
		opts = nullptr;
	}

	if (codec_ctx) {
		avcodec_free_context(&codec_ctx);
		codec_ctx = nullptr;
	}

	CleanupHardwareAcceleration();

	if (fmt_ctx) {
		avformat_close_input(&fmt_ctx);
		fmt_ctx = nullptr;
	}

	avstream = nullptr;
}

int fb_decoder_open(FBDecoder *decoder, const char *filename, int stream_index)
{
	if (!decoder || !filename) {
		return AVERROR(EINVAL);
	}
	return decoder->Open(filename, stream_index) ? 0 : AVERROR_EXTERNAL;
}

void fb_decoder_close(FBDecoder *decoder)
{
	if (decoder) {
		decoder->Close();
	}
}

int fb_decoder_get_frame(FBDecoder *decoder, FBPacket *packet, FBFrame *frame)
{
	if (!decoder || !decoder->codec_ctx || !packet || !frame) {
		return AVERROR(EINVAL);
	}

	AVPacket *pkt = packet->pkt;
	AVFrame *frm = frame->frame;

	bool eof = false;
	int ret;

	// Clear any previous frames
	av_frame_unref(frm);

	while ((ret = avcodec_receive_frame(decoder->codec_ctx, frm)) ==
			   AVERROR(EAGAIN) &&
		   !eof) {
		// Find next packet in the correct stream index
		ret = fb_decoder_get_packet(decoder, packet);

		if (ret == AVERROR_EOF) {
			// Don't break so that receive gets called again, but don't try to read again
			eof = true;

			// Send a null packet to signal end of stream
			avcodec_send_packet(decoder->codec_ctx, nullptr);
		} else if (ret < 0) {
			// Handle other error by breaking loop and returning the code we received
			break;
		} else {
			// Successful read, send the packet
			ret = avcodec_send_packet(decoder->codec_ctx, pkt);

			// We don't need the packet anymore, so free it
			av_packet_unref(pkt);

			if (ret < 0) {
				break;
			}
		}
	}

	return ret;
}

int fb_decoder_get_packet(FBDecoder *decoder, FBPacket *packet)
{
	if (!decoder || !decoder->fmt_ctx || !packet) {
		return AVERROR(EINVAL);
	}

	AVPacket *pkt = packet->pkt;
	int ret;

	do {
		av_packet_unref(pkt);
		ret = av_read_frame(decoder->fmt_ctx, pkt);
	} while (pkt->stream_index != decoder->avstream->index && ret >= 0);

	return ret;
}

void fb_decoder_seek(FBDecoder *decoder, int64_t timestamp)
{
	if (!decoder || !decoder->fmt_ctx) {
		return;
	}

	avcodec_flush_buffers(decoder->codec_ctx);
	av_seek_frame(decoder->fmt_ctx, decoder->avstream->index, timestamp,
				  AVSEEK_FLAG_BACKWARD);
}

int fb_decoder_get_stream_info(const FBDecoder *decoder, FBStreamInfo *out)
{
	if (!decoder || !decoder->avstream || !out) {
		return AVERROR(EINVAL);
	}

	const AVStream *s = decoder->avstream;
	const AVCodecParameters *par = s->codecpar;

	memset(out, 0, sizeof(*out));
	out->index = s->index;
	out->codec_type = par->codec_type;
	out->codec_id = par->codec_id;
	out->has_decoder = 1; // stream is open, so a decoder was found
	out->width = par->width;
	out->height = par->height;
	out->pixel_format = fb::PixFmtFromAV(AVPixelFormat(par->format));
	out->field_order = decoder->codec_ctx ? decoder->codec_ctx->field_order :
											AV_FIELD_UNKNOWN;
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

	return 0;
}

int64_t fb_decoder_get_format_start_time(const FBDecoder *decoder)
{
	if (!decoder || !decoder->fmt_ctx) {
		return FB_NOPTS_VALUE;
	}
	return decoder->fmt_ctx->start_time;
}

int64_t fb_decoder_get_format_duration(const FBDecoder *decoder)
{
	if (!decoder || !decoder->fmt_ctx) {
		return FB_NOPTS_VALUE;
	}
	return decoder->fmt_ctx->duration;
}

int fb_decoder_guess_sample_aspect_ratio(const FBDecoder *decoder,
										 FBFrame *frame, int *num, int *den)
{
	if (!decoder || !decoder->fmt_ctx || !num || !den) {
		return AVERROR(EINVAL);
	}

	AVRational r = av_guess_sample_aspect_ratio(
		decoder->fmt_ctx, decoder->avstream, frame ? frame->frame : nullptr);
	*num = r.num;
	*den = r.den;
	return 0;
}

int fb_decoder_guess_frame_rate(const FBDecoder *decoder, FBFrame *frame,
								int *num, int *den)
{
	if (!decoder || !decoder->fmt_ctx || !num || !den) {
		return AVERROR(EINVAL);
	}

	AVRational r = av_guess_frame_rate(decoder->fmt_ctx, decoder->avstream,
									   frame ? frame->frame : nullptr);
	*num = r.num;
	*den = r.den;
	return 0;
}

int fb_decoder_hwaccel_enabled(const FBDecoder *decoder)
{
	return decoder && decoder->hwaccel_enabled;
}

int fb_decoder_hw_pix_fmt(const FBDecoder *decoder)
{
	// Hardware pixel formats have no static FB_PIX_FMT_* identifier, so this
	// returns a process-local dynamic id for them. Use fb_frame_is_hw() to
	// detect hardware frames.
	return decoder ? fb::PixFmtFromAV(decoder->hw_pix_fmt) : FB_PIX_FMT_NONE;
}
