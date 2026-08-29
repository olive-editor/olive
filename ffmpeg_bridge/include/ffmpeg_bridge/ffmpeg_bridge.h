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

#ifndef FFMPEG_BRIDGE_H
#define FFMPEG_BRIDGE_H

/**
 * ffmpeg_bridge - pure C API isolating all FFmpeg access from the editor.
 *
 * All objects (frames, packets, decoders, encoders, scalers, resamplers,
 * audio graphs) are identified by opaque handles and always live inside the
 * shared library; callers never see or dereference an FFmpeg structure.
 *
 * Error convention: functions returning int return 0 (or a non-negative
 * value) on success and a negative error code on failure. Negative codes are
 * FFmpeg error codes and can be converted to text with fb_error_string().
 * End-of-file is reported as FB_ERROR_EOF.
 */

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#if defined(FFMPEG_BRIDGE_BUILD)
#define FB_API __declspec(dllexport)
#else
#define FB_API __declspec(dllimport)
#endif
#else
#define FB_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Constants                                                                  */
/* ------------------------------------------------------------------------- */

/** Pass-through of AVERROR_EOF (verified by static_assert in the library). */
#define FB_ERROR_EOF (-541478725)

/** Pass-through of AV_NOPTS_VALUE. */
#define FB_NOPTS_VALUE INT64_MIN

/** Pass-through of AV_TIME_BASE. */
#define FB_TIME_BASE 1000000

/** Scaler algorithm flags (values mirror libswscale). */
#define FB_SCALER_POINT 0x10

/**
 * Pixel formats. These are fixed identifiers; the library maps them to the
 * AVPixelFormat values of whatever FFmpeg build it was compiled against
 * (AVPixelFormat enum values shift between FFmpeg releases). Callers must
 * treat them as opaque. A format may be unknown to an older FFmpeg build,
 * in which case library calls reject it gracefully. Decoded frames whose
 * format has no entry here receive an opaque process-local id >= 1000.
 */
typedef enum FBPixelFormat {
	FB_PIX_FMT_NONE = -1,
	FB_PIX_FMT_YUV420P = 0,
	FB_PIX_FMT_RGB24 = 2,
	FB_PIX_FMT_YUV422P = 4,
	FB_PIX_FMT_YUV444P = 5,
	FB_PIX_FMT_YUV410P = 6,
	FB_PIX_FMT_YUV411P = 7,
	FB_PIX_FMT_GRAY8 = 8,
	FB_PIX_FMT_YUVJ420P = 12,
	FB_PIX_FMT_YUVJ422P = 13,
	FB_PIX_FMT_YUVJ444P = 14,
	FB_PIX_FMT_NV12 = 23,
	FB_PIX_FMT_RGBA = 26,
	FB_PIX_FMT_GRAY16LE = 30,
	FB_PIX_FMT_YUV440P = 31,
	FB_PIX_FMT_YUVJ440P = 32,
	FB_PIX_FMT_RGB48LE = 35,
	FB_PIX_FMT_YUV420P10LE = 62,
	FB_PIX_FMT_YUV422P10LE = 64,
	FB_PIX_FMT_YUV444P10LE = 68,
	FB_PIX_FMT_RGBA64LE = 105,
	FB_PIX_FMT_YUV420P12LE = 123,
	FB_PIX_FMT_YUV422P12LE = 127,
	FB_PIX_FMT_YUV444P12LE = 131,
	FB_PIX_FMT_YUVJ411P = 138,
	FB_PIX_FMT_P010LE = 158,
	FB_PIX_FMT_GRAYF32LE = 183,
	FB_PIX_FMT_RGBAF16LE = 207,
	FB_PIX_FMT_RGBF32LE = 218,
	FB_PIX_FMT_RGBAF32LE = 220,
	FB_PIX_FMT_RGBF16LE = 234,
	FB_PIX_FMT_GRAYF16LE = 248
} FBPixelFormat;

/**
 * Sample formats. Values mirror AVSampleFormat (static_assert'ed).
 */
typedef enum FBSampleFormat {
	FB_SAMPLE_FMT_NONE = -1,
	FB_SAMPLE_FMT_U8 = 0,
	FB_SAMPLE_FMT_S16 = 1,
	FB_SAMPLE_FMT_S32 = 2,
	FB_SAMPLE_FMT_FLT = 3,
	FB_SAMPLE_FMT_DBL = 4,
	FB_SAMPLE_FMT_U8P = 5,
	FB_SAMPLE_FMT_S16P = 6,
	FB_SAMPLE_FMT_S32P = 7,
	FB_SAMPLE_FMT_FLTP = 8,
	FB_SAMPLE_FMT_DBLP = 9,
	FB_SAMPLE_FMT_S64 = 10,
	FB_SAMPLE_FMT_S64P = 11
} FBSampleFormat;

/** Color ranges. Values mirror AVColorRange (static_assert'ed). */
typedef enum FBColorRange {
	FB_COLOR_RANGE_UNSPEC = 0,
	FB_COLOR_RANGE_MPEG = 1,
	FB_COLOR_RANGE_JPEG = 2
} FBColorRange;

/** Color spaces. Values mirror AVColorSpace (static_assert'ed). */
typedef enum FBColorSpace {
	FB_COL_SPC_RGB = 0,
	FB_COL_SPC_BT709 = 1,
	FB_COL_SPC_UNSPEC = 2,
	FB_COL_SPC_FCC = 4,
	FB_COL_SPC_BT470BG = 5,
	FB_COL_SPC_SMPTE170M = 6,
	FB_COL_SPC_SMPTE240M = 7,
	FB_COL_SPC_BT2020_NCL = 9
} FBColorSpace;

/** Media types. Values mirror AVMediaType (static_assert'ed). */
typedef enum FBMediaType {
	FB_MEDIA_TYPE_VIDEO = 0,
	FB_MEDIA_TYPE_AUDIO = 1,
	FB_MEDIA_TYPE_DATA = 2,
	FB_MEDIA_TYPE_SUBTITLE = 3
} FBMediaType;

/** Field orders. Values mirror AVFieldOrder (static_assert'ed). */
typedef enum FBFieldOrder {
	FB_FIELD_ORDER_UNKNOWN = 0,
	FB_FIELD_ORDER_PROGRESSIVE = 1,
	FB_FIELD_ORDER_TT = 2,
	FB_FIELD_ORDER_BB = 3,
	FB_FIELD_ORDER_TB = 4,
	FB_FIELD_ORDER_BT = 5
} FBFieldOrder;

/**
 * Channel layout masks. Values mirror AV_CH_LAYOUT_* (static_assert'ed).
 */
#define FB_CH_LAYOUT_MONO ((uint64_t)0x4)
#define FB_CH_LAYOUT_STEREO ((uint64_t)0x3)
#define FB_CH_LAYOUT_2_1 ((uint64_t)0x103)
#define FB_CH_LAYOUT_5POINT1 ((uint64_t)0x60F)
#define FB_CH_LAYOUT_7POINT1 ((uint64_t)0x63F)

/**
 * Codecs the encoder supports. Opaque ordering; mapped explicitly by the
 * caller's adapter layer (no value relationship with any app-side enum).
 */
typedef enum FBCodec {
	FB_CODEC_NONE = -1,
	FB_CODEC_H264 = 0,
	FB_CODEC_H264RGB,
	FB_CODEC_DNXHD,
	FB_CODEC_PRORES,
	FB_CODEC_CINEFORM,
	FB_CODEC_H265,
	FB_CODEC_VP9,
	FB_CODEC_AV1,
	FB_CODEC_OPENEXR,
	FB_CODEC_PNG,
	FB_CODEC_TIFF,
	FB_CODEC_MP2,
	FB_CODEC_MP3,
	FB_CODEC_AAC,
	FB_CODEC_PCM,
	FB_CODEC_FLAC,
	FB_CODEC_OPUS,
	FB_CODEC_VORBIS,
	FB_CODEC_SRT
} FBCodec;

/** Cancellation callback: return non-zero to request cancellation. */
typedef int (*FBCancelCallback)(void *userdata);

/* ------------------------------------------------------------------------- */
/* Opaque handles                                                             */
/* ------------------------------------------------------------------------- */

typedef struct FBFrame FBFrame;
typedef struct FBPacket FBPacket;
typedef struct FBDecoder FBDecoder;
typedef struct FBProbe FBProbe;
typedef struct FBEncoder FBEncoder;
typedef struct FBScaler FBScaler;
typedef struct FBResampler FBResampler;
typedef struct FBAudioGraph FBAudioGraph;

/* ------------------------------------------------------------------------- */
/* Error strings / version                                                    */
/* ------------------------------------------------------------------------- */

FB_API void fb_error_string(int error_code, char *buffer, int buffer_size);
FB_API const char *fb_version_string(void);

/* ------------------------------------------------------------------------- */
/* Pixel/sample format utilities                                              */
/* ------------------------------------------------------------------------- */

FB_API const char *fb_pix_fmt_name(int pix_fmt);
FB_API int fb_pix_fmt_from_name(const char *name);
FB_API int fb_pix_fmt_bits_per_pixel(int pix_fmt);
FB_API int fb_pix_fmt_has_alpha(int pix_fmt);
FB_API int fb_pix_fmt_is_planar(int pix_fmt);
/** Size in bytes of one sample component (1 for 8-bit formats, 2 for 9-16bit). */
FB_API int fb_pix_fmt_component_size(int pix_fmt);

/**
 * Find the best pixel format from `list` for storing `pix_fmt` losslessly.
 * `list` is terminated by FB_PIX_FMT_NONE. Mirrors
 * avcodec_find_best_pix_fmt_of_list(list, fmt, has_alpha=1).
 */
FB_API int fb_find_best_pix_fmt_of_list(const int *list, int pix_fmt);

/** Number of channels in a channel layout mask. */
FB_API int fb_channel_layout_get_channels(uint64_t mask);
/** Default layout mask for a channel count (mirrors av_channel_layout_default). */
FB_API uint64_t fb_channel_layout_default(int nb_channels);

/* ------------------------------------------------------------------------- */
/* Frame (AVFrame wrapper)                                                    */
/* ------------------------------------------------------------------------- */

FB_API FBFrame *fb_frame_alloc(void);
FB_API void fb_frame_free(FBFrame **frame);
/** Clear the frame's contents back to defaults (mirrors av_frame_unref). */
FB_API void fb_frame_unref(FBFrame *frame);
/** Allocate the frame's buffer(s) from its width/height/format fields. */
FB_API int fb_frame_get_buffer(FBFrame *frame, int align);
/** Ensure the frame's data is writable (mirrors av_frame_make_writable). */
FB_API int fb_frame_make_writable(FBFrame *frame);
FB_API int fb_frame_copy_props(FBFrame *dst, const FBFrame *src);
/** Transfer data between a hardware frame and a software frame. */
FB_API int fb_frame_hw_transfer_data(FBFrame *dst, const FBFrame *src);
FB_API int fb_frame_is_hw(const FBFrame *frame);

FB_API int fb_frame_get_width(const FBFrame *frame);
FB_API void fb_frame_set_width(FBFrame *frame, int width);
FB_API int fb_frame_get_height(const FBFrame *frame);
FB_API void fb_frame_set_height(FBFrame *frame, int height);
FB_API int fb_frame_get_format(const FBFrame *frame);
FB_API void fb_frame_set_format(FBFrame *frame, int format);
FB_API int64_t fb_frame_get_pts(const FBFrame *frame);
FB_API void fb_frame_set_pts(FBFrame *frame, int64_t pts);
FB_API int64_t fb_frame_get_best_effort_timestamp(const FBFrame *frame);
FB_API int fb_frame_get_nb_samples(const FBFrame *frame);
FB_API void fb_frame_set_nb_samples(FBFrame *frame, int nb_samples);
FB_API int fb_frame_get_sample_rate(const FBFrame *frame);
FB_API void fb_frame_set_sample_rate(FBFrame *frame, int sample_rate);
FB_API int fb_frame_get_color_range(const FBFrame *frame);
FB_API void fb_frame_set_color_range(FBFrame *frame, int color_range);
FB_API int fb_frame_get_colorspace(const FBFrame *frame);
FB_API void fb_frame_set_colorspace(FBFrame *frame, int colorspace);
FB_API uint64_t fb_frame_get_channel_layout_mask(const FBFrame *frame);
FB_API void fb_frame_set_channel_layout_mask(FBFrame *frame, uint64_t mask);

FB_API uint8_t *fb_frame_get_data(FBFrame *frame, int plane);
FB_API const uint8_t *fb_frame_get_data_const(const FBFrame *frame, int plane);
FB_API void fb_frame_set_data(FBFrame *frame, int plane, uint8_t *data);
FB_API int fb_frame_get_linesize(const FBFrame *frame, int plane);
FB_API void fb_frame_set_linesize(FBFrame *frame, int plane, int linesize);

/* ------------------------------------------------------------------------- */
/* Packet (AVPacket wrapper)                                                  */
/* ------------------------------------------------------------------------- */

FB_API FBPacket *fb_packet_alloc(void);
FB_API void fb_packet_free(FBPacket **packet);
FB_API void fb_packet_unref(FBPacket *packet);

FB_API int64_t fb_packet_get_pts(const FBPacket *packet);
FB_API int64_t fb_packet_get_duration(const FBPacket *packet);
FB_API int fb_packet_get_size(const FBPacket *packet);
FB_API const uint8_t *fb_packet_get_data(const FBPacket *packet);
FB_API int fb_packet_get_stream_index(const FBPacket *packet);

/* ------------------------------------------------------------------------- */
/* Stream info                                                                */
/* ------------------------------------------------------------------------- */

typedef struct FBStreamInfo {
	int index;
	int codec_type;   /* FBMediaType */
	int codec_id;     /* opaque FFmpeg codec id */
	int has_decoder;  /* non-zero if a decoder exists for this stream */

	int width;
	int height;
	int pixel_format; /* FBPixelFormat */
	int field_order;  /* FBFieldOrder */
	int color_range;  /* FBColorRange */

	int sample_rate;
	int sample_format; /* FBSampleFormat */
	uint64_t channel_layout_mask; /* validated (never zero for valid audio) */

	int64_t start_time;
	int64_t duration;
	int time_base_num;
	int time_base_den;
	int avg_frame_rate_num;
	int avg_frame_rate_den;
} FBStreamInfo;

/* ------------------------------------------------------------------------- */
/* Decoder                                                                    */
/* ------------------------------------------------------------------------- */

FB_API FBDecoder *fb_decoder_create(void);
FB_API void fb_decoder_free(FBDecoder **decoder);

FB_API int fb_decoder_open(FBDecoder *decoder, const char *filename,
						   int stream_index);
FB_API void fb_decoder_close(FBDecoder *decoder);

/**
 * Retrieve the next decoded frame. Returns 0 on success, FB_ERROR_EOF at end
 * of stream, or another negative error code.
 */
FB_API int fb_decoder_get_frame(FBDecoder *decoder, FBPacket *packet,
								FBFrame *frame);
/** Retrieve the next raw packet of the opened stream. Same return contract. */
FB_API int fb_decoder_get_packet(FBDecoder *decoder, FBPacket *packet);
FB_API void fb_decoder_seek(FBDecoder *decoder, int64_t timestamp);

FB_API int fb_decoder_get_stream_info(const FBDecoder *decoder,
									  FBStreamInfo *out);
FB_API int64_t fb_decoder_get_format_start_time(const FBDecoder *decoder);
FB_API int64_t fb_decoder_get_format_duration(const FBDecoder *decoder);

FB_API int fb_decoder_guess_sample_aspect_ratio(const FBDecoder *decoder,
												FBFrame *frame, int *num,
												int *den);
FB_API int fb_decoder_guess_frame_rate(const FBDecoder *decoder,
									   FBFrame *frame, int *num, int *den);

FB_API int fb_decoder_hwaccel_enabled(const FBDecoder *decoder);
FB_API int fb_decoder_hw_pix_fmt(const FBDecoder *decoder);

/* ------------------------------------------------------------------------- */
/* Probe                                                                      */
/* ------------------------------------------------------------------------- */

FB_API FBProbe *fb_probe_create(void);
FB_API void fb_probe_free(FBProbe **probe);

FB_API int fb_probe_open(FBProbe *probe, const char *filename);
FB_API void fb_probe_close(FBProbe *probe);

FB_API int fb_probe_get_stream_count(const FBProbe *probe);
FB_API int fb_probe_get_stream_info(const FBProbe *probe, int stream_index,
									FBStreamInfo *out);
FB_API int64_t fb_probe_get_duration(const FBProbe *probe);
FB_API int64_t fb_probe_get_start_time(const FBProbe *probe);
FB_API int fb_probe_duration_from_bitrate(const FBProbe *probe);

/**
 * Read a metadata value from the format (stream_index = -1) or from a stream.
 * Returns 1 if found (buffer filled), 0 otherwise.
 */
FB_API int fb_probe_get_metadata(FBProbe *probe, int stream_index,
								 const char *key, char *buffer,
								 int buffer_size);

typedef struct FBVideoStreamDetails {
	int field_order; /* FBFieldOrder */
	int pixel_aspect_num;
	int pixel_aspect_den;
	int frame_rate_num;
	int frame_rate_den;
	int is_still;
	int64_t decoded_duration; /* last timestamp seen, FB_NOPTS_VALUE if none */
} FBVideoStreamDetails;

/**
 * Open the file and decode the start of a video stream to determine
 * interlacing, aspect ratio, frame rate, whether it is a still image, and
 * (when `decode_full_duration` is non-zero, by decoding to the end) its true
 * duration.
 */
FB_API int fb_probe_video_stream_details(const char *filename, int stream_index,
										 FBVideoStreamDetails *out,
										 int decode_full_duration,
										 FBCancelCallback cancel,
										 void *cancel_userdata);

/** Decode an audio stream to its end to determine its true duration. */
FB_API int fb_probe_audio_stream_duration(const char *filename,
										  int stream_index,
										  int64_t *out_duration,
										  FBCancelCallback cancel,
										  void *cancel_userdata);

/** Called once per subtitle packet. */
typedef void (*FBSubtitleCallback)(int64_t pts, int64_t duration,
								   const char *text, int text_size,
								   void *userdata);

/** Read all subtitle packets of a stream (e.g. SRT) via callback. */
FB_API int fb_probe_read_subtitle_stream(const char *filename, int stream_index,
										 FBSubtitleCallback callback,
										 void *userdata);

/* ------------------------------------------------------------------------- */
/* Scaler (libswscale wrapper)                                                */
/* ------------------------------------------------------------------------- */

FB_API FBScaler *fb_scaler_create(int src_width, int src_height, int src_format,
								  int dst_width, int dst_height, int dst_format,
								  int flags);
FB_API void fb_scaler_free(FBScaler **scaler);
/**
 * Set colorspace details from an AVColorSpace value; the appropriate swscale
 * coefficient table is looked up internally. `jpeg_range` is non-zero for
 * full-range (JPEG) content.
 */
FB_API int fb_scaler_set_colorspace(FBScaler *scaler, int colorspace,
									int jpeg_range);
FB_API int fb_scaler_scale_frame(FBScaler *scaler, FBFrame *dst,
								 const FBFrame *src);
FB_API int fb_scaler_scale_slices(FBScaler *scaler,
								  const uint8_t *const *src_data,
								  const int *src_linesize, int src_height,
								  uint8_t *const *dst_data,
								  const int *dst_linesize);

/**
 * YUV->RGB conversion coefficients for `colorspace`, normalized to [0,1].
 * Output order: crv, cbu, cgu, cgv.
 */
FB_API void fb_get_yuv_coefficients(int colorspace, double out[4]);

/* ------------------------------------------------------------------------- */
/* Resampler (libswresample wrapper)                                          */
/* ------------------------------------------------------------------------- */

FB_API FBResampler *fb_resampler_create(uint64_t out_layout_mask, int out_format,
										int out_rate, uint64_t in_layout_mask,
										int in_format, int in_rate);
FB_API void fb_resampler_free(FBResampler **resampler);
FB_API int fb_resampler_get_out_samples(FBResampler *resampler,
										int in_samples);
FB_API int fb_resampler_convert(FBResampler *resampler, uint8_t **out,
								int out_count, const uint8_t **in,
								int in_count);
/** Same as fb_resampler_convert but takes the input directly from a frame. */
FB_API int fb_resampler_convert_frame(FBResampler *resampler, uint8_t **out,
									  int out_count, const FBFrame *in_frame);

/* ------------------------------------------------------------------------- */
/* Audio filter graph (abuffer/aformat/atempo/abuffersink wrapper)            */
/* ------------------------------------------------------------------------- */

typedef struct FBAudioGraphConfig {
	int in_sample_rate;
	uint64_t in_channel_layout_mask; /* 0 = derive default from in_channels */
	int in_sample_format;            /* FBSampleFormat (planar float in) */
	int in_channels;

	int out_sample_rate;
	uint64_t out_channel_layout_mask; /* 0 = derive default from out_channels */
	int out_sample_format;
	int out_channels;
	int out_is_planar;

	double tempo;
} FBAudioGraphConfig;

FB_API FBAudioGraph *fb_audio_graph_create(const FBAudioGraphConfig *config);
FB_API void fb_audio_graph_free(FBAudioGraph **graph);
/** Push planar samples into the graph. channel_data == NULL flushes the graph. */
FB_API int fb_audio_graph_push(FBAudioGraph *graph,
							   const uint8_t *const *channel_data,
							   int nb_samples);
/** Pull converted samples. Returns 1 if a frame was produced, 0 if more input
 * is needed, or a negative error code. */
FB_API int fb_audio_graph_pull(FBAudioGraph *graph, FBFrame *out_frame);

/* ------------------------------------------------------------------------- */
/* Encoder                                                                    */
/* ------------------------------------------------------------------------- */

typedef struct FBEncoderConfig {
	const char *filename;

	int video_enabled;
	int video_codec; /* FBCodec */
	int video_width;
	int video_height;
	int video_pixel_aspect_num;
	int video_pixel_aspect_den;
	int video_time_base_num; /* frame rate expressed as a time base */
	int video_time_base_den;
	int video_frame_rate_num;
	int video_frame_rate_den;
	const char *video_pix_fmt; /* encoder pixel format name, e.g. "yuv420p" */
	int video_src_pix_fmt; /* FBPixelFormat of frames passed to write_video_frame */
	int video_color_range;     /* FBColorRange */
	int video_field_order;     /* FBFieldOrder (PROGRESSIVE/TT/BB) */
	int64_t video_bit_rate;
	int64_t video_min_bit_rate;
	int64_t video_max_bit_rate;
	int64_t video_buffer_size;
	int video_threads;
	int video_color_srgb; /* non-zero: tag nclc as sRGB, else Rec.709 */
	const char **video_opt_keys;
	const char **video_opt_values;
	int video_opt_count;

	int audio_enabled;
	int audio_codec; /* FBCodec */
	int audio_sample_rate;
	uint64_t audio_channel_layout_mask;
	int audio_sample_format; /* FBSampleFormat */
	int64_t audio_bit_rate;

	int subtitles_enabled;
	int subtitle_codec; /* FBCodec */
	const uint8_t *subtitle_header;
	int subtitle_header_size;
} FBEncoderConfig;

FB_API FBEncoder *fb_encoder_create(const FBEncoderConfig *config);
FB_API void fb_encoder_free(FBEncoder **encoder);

FB_API int fb_encoder_open(FBEncoder *encoder);
FB_API void fb_encoder_close(FBEncoder *encoder);

/**
 * Write one video frame of raw pixel data. `time_seconds` is the presentation
 * time in seconds. The data must remain valid until this call returns.
 */
FB_API int fb_encoder_write_video_frame(FBEncoder *encoder, int width,
										int height, int pix_fmt,
										const uint8_t *data, int linesize,
										double time_seconds);

/**
 * Write planar audio samples. `channel_data` has `channels` pointers, each
 * with `sample_count` samples of `sample_format`. Passing sample_count == 0
 * flushes the audio encoder.
 */
FB_API int fb_encoder_write_audio(FBEncoder *encoder,
								  const uint8_t *const *channel_data,
								  int channels, int sample_format,
								  int sample_rate, uint64_t channel_layout_mask,
								  int64_t sample_count);

FB_API int fb_encoder_write_subtitle(FBEncoder *encoder, const char *utf8_text,
									 double in_seconds, double duration_seconds);

/** Last error message (empty string if none). Valid until the next call. */
FB_API const char *fb_encoder_get_error(const FBEncoder *encoder);

/**
 * List the pixel formats an encoder codec supports.
 * Writes up to `max_names` names into `names`, returns the total count.
 */
FB_API int fb_encoder_codec_get_pixel_formats(int codec, const char **names,
											  int max_names);
/**
 * List the sample formats (FBSampleFormat) an encoder codec supports.
 * Writes up to `max_fmts` into `fmts`, returns the total count.
 */
FB_API int fb_encoder_codec_get_sample_formats(int codec, int *fmts,
											   int max_fmts);

#ifdef __cplusplus
}
#endif

#endif // FFMPEG_BRIDGE_H
