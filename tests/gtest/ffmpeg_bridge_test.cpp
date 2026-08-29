/*
 * Oak Video Editor - ffmpeg_bridge C API Tests
 * Copyright (C) 2025 Olive CE Team
 *
 * Direct tests of the pure C ffmpeg_bridge API. These tests exercise the
 * bridge in isolation from the editor: handles are created, used and freed
 * entirely through the fb_* functions, mirroring how the C++ adapter layer
 * drives the library at runtime.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <ffmpeg_bridge/ffmpeg_bridge.h>

#include "olive/core/render/channellayout.h"

namespace
{

QString DemoPath()
{
	return QDir(QStringLiteral(OAK_TEST_SOURCE_DIR))
		.filePath(QStringLiteral("tests/demo.mp4"));
}

QString TempFilePath(const QString &name)
{
	return QDir::temp().filePath(name);
}

constexpr double kPi = 3.14159265358979323846;

// Finds the first stream of `type`; returns its index or -1.
int FindStream(FBProbe *probe, int type)
{
	const int count = fb_probe_get_stream_count(probe);
	for (int i = 0; i < count; ++i) {
		FBStreamInfo info;
		if (fb_probe_get_stream_info(probe, i, &info) == 0 &&
			info.codec_type == type) {
			return i;
		}
	}
	return -1;
}

} // namespace

// ============================================================================
// Constants
// ============================================================================

TEST(FFmpegBridgeConstants, ChannelLayoutsMatchCore)
{
	// The core library and the bridge must agree on layout masks or all
	// audio plumbing between them breaks.
	EXPECT_EQ(FB_CH_LAYOUT_MONO, olive::core::kChannelLayoutMono);
	EXPECT_EQ(FB_CH_LAYOUT_STEREO, olive::core::kChannelLayoutStereo);
	EXPECT_EQ(FB_CH_LAYOUT_2_1, olive::core::kChannelLayout2_1);
	EXPECT_EQ(FB_CH_LAYOUT_5POINT1, olive::core::kChannelLayout5Point1);
	EXPECT_EQ(FB_CH_LAYOUT_7POINT1, olive::core::kChannelLayout7Point1);
}

TEST(FFmpegBridgeConstants, SpecialValues)
{
	EXPECT_EQ(FB_NOPTS_VALUE, INT64_MIN);
	EXPECT_EQ(FB_TIME_BASE, 1000000);
	EXPECT_LT(FB_ERROR_EOF, 0);
	EXPECT_EQ(FB_PIX_FMT_NONE, -1);
	EXPECT_EQ(FB_SAMPLE_FMT_NONE, -1);
}

// ============================================================================
// Error strings / version
// ============================================================================

TEST(FFmpegBridgeError, ErrorString)
{
	char buffer[256];
	fb_error_string(FB_ERROR_EOF, buffer, sizeof(buffer));
	EXPECT_GT(std::strlen(buffer), 0u);

	fb_error_string(0, buffer, sizeof(buffer));
	EXPECT_GT(std::strlen(buffer), 0u);
}

TEST(FFmpegBridgeError, VersionString)
{
	const char *version = fb_version_string();
	ASSERT_NE(version, nullptr);
	EXPECT_GT(std::strlen(version), 0u);
}

// ============================================================================
// Pixel/sample format utilities
// ============================================================================

TEST(FFmpegBridgePixFmt, NameRoundtrip)
{
	const char *name = fb_pix_fmt_name(FB_PIX_FMT_YUV420P);
	ASSERT_NE(name, nullptr);
	EXPECT_STREQ(name, "yuv420p");
	EXPECT_EQ(fb_pix_fmt_from_name(name), FB_PIX_FMT_YUV420P);

	EXPECT_STREQ(fb_pix_fmt_name(FB_PIX_FMT_RGBA), "rgba");
	EXPECT_EQ(fb_pix_fmt_from_name("rgba"), FB_PIX_FMT_RGBA);
}

TEST(FFmpegBridgePixFmt, Properties)
{
	EXPECT_EQ(fb_pix_fmt_bits_per_pixel(FB_PIX_FMT_RGBA), 32);
	EXPECT_EQ(fb_pix_fmt_bits_per_pixel(FB_PIX_FMT_YUV420P), 12);
	EXPECT_EQ(fb_pix_fmt_bits_per_pixel(FB_PIX_FMT_RGBA64LE), 64);

	EXPECT_EQ(fb_pix_fmt_has_alpha(FB_PIX_FMT_RGBA), 1);
	EXPECT_EQ(fb_pix_fmt_has_alpha(FB_PIX_FMT_YUV420P), 0);

	EXPECT_EQ(fb_pix_fmt_is_planar(FB_PIX_FMT_YUV420P), 1);
	EXPECT_EQ(fb_pix_fmt_is_planar(FB_PIX_FMT_RGBA), 0);

	EXPECT_EQ(fb_pix_fmt_component_size(FB_PIX_FMT_RGBA), 1);
	EXPECT_EQ(fb_pix_fmt_component_size(FB_PIX_FMT_RGBA64LE), 2);
}

TEST(FFmpegBridgePixFmt, FindBestOfList)
{
	const int list[] = { FB_PIX_FMT_YUV420P, FB_PIX_FMT_YUV444P,
						 FB_PIX_FMT_NONE };
	EXPECT_EQ(fb_find_best_pix_fmt_of_list(list, FB_PIX_FMT_YUV420P),
			  FB_PIX_FMT_YUV420P);
}

TEST(FFmpegBridgeChannelLayout, ChannelCounts)
{
	EXPECT_EQ(fb_channel_layout_get_channels(FB_CH_LAYOUT_MONO), 1);
	EXPECT_EQ(fb_channel_layout_get_channels(FB_CH_LAYOUT_STEREO), 2);
	EXPECT_EQ(fb_channel_layout_get_channels(FB_CH_LAYOUT_5POINT1), 6);
	EXPECT_EQ(fb_channel_layout_get_channels(FB_CH_LAYOUT_7POINT1), 8);
}

TEST(FFmpegBridgeChannelLayout, Defaults)
{
	EXPECT_EQ(fb_channel_layout_default(1), FB_CH_LAYOUT_MONO);
	EXPECT_EQ(fb_channel_layout_default(2), FB_CH_LAYOUT_STEREO);
	// av_channel_layout_default picks a valid layout with the right channel
	// count, but not necessarily the same variant as the named constants
	EXPECT_EQ(fb_channel_layout_get_channels(fb_channel_layout_default(6)), 6);
	EXPECT_NE(fb_channel_layout_default(3), (uint64_t)0);
}

// ============================================================================
// Frame
// ============================================================================

TEST(FFmpegBridgeFrame, AllocAndFields)
{
	FBFrame *frame = fb_frame_alloc();
	ASSERT_NE(frame, nullptr);

	EXPECT_EQ(fb_frame_get_width(frame), 0);
	EXPECT_EQ(fb_frame_get_format(frame), FB_PIX_FMT_NONE);

	fb_frame_set_width(frame, 320);
	fb_frame_set_height(frame, 240);
	fb_frame_set_format(frame, FB_PIX_FMT_RGBA);
	fb_frame_set_pts(frame, 12345);

	EXPECT_EQ(fb_frame_get_width(frame), 320);
	EXPECT_EQ(fb_frame_get_height(frame), 240);
	EXPECT_EQ(fb_frame_get_format(frame), FB_PIX_FMT_RGBA);
	EXPECT_EQ(fb_frame_get_pts(frame), 12345);

	fb_frame_free(&frame);
	EXPECT_EQ(frame, nullptr);
}

TEST(FFmpegBridgeFrame, BufferAllocAndAccess)
{
	FBFrame *frame = fb_frame_alloc();
	ASSERT_NE(frame, nullptr);

	fb_frame_set_width(frame, 64);
	fb_frame_set_height(frame, 48);
	fb_frame_set_format(frame, FB_PIX_FMT_RGBA);
	ASSERT_EQ(fb_frame_get_buffer(frame, 0), 0);
	ASSERT_EQ(fb_frame_make_writable(frame), 0);

	EXPECT_NE(fb_frame_get_data(frame, 0), nullptr);
	EXPECT_GE(fb_frame_get_linesize(frame, 0), 64 * 4);
	EXPECT_EQ(fb_frame_is_hw(frame), 0);

	// Write a pattern through the API and read it back
	const int linesize = fb_frame_get_linesize(frame, 0);
	for (int y = 0; y < 48; ++y) {
		std::memset(fb_frame_get_data(frame, 0) + y * linesize, y & 0xFF,
					linesize);
	}
	const uint8_t *data = fb_frame_get_data_const(frame, 0);
	ASSERT_NE(data, nullptr);
	EXPECT_EQ(data[0], 0);
	EXPECT_EQ(data[10 * linesize], 10);

	// Out-of-range plane access is safe
	EXPECT_EQ(fb_frame_get_data(frame, -1), nullptr);
	EXPECT_EQ(fb_frame_get_data(frame, 8), nullptr);

	fb_frame_unref(frame);
	EXPECT_EQ(fb_frame_get_data(frame, 0), nullptr);

	fb_frame_free(&frame);
}

TEST(FFmpegBridgeFrame, AudioFields)
{
	FBFrame *frame = fb_frame_alloc();
	ASSERT_NE(frame, nullptr);

	fb_frame_set_nb_samples(frame, 1024);
	fb_frame_set_sample_rate(frame, 44100);
	fb_frame_set_format(frame, FB_SAMPLE_FMT_FLTP);
	fb_frame_set_channel_layout_mask(frame, FB_CH_LAYOUT_STEREO);
	ASSERT_EQ(fb_frame_get_buffer(frame, 0), 0);

	EXPECT_EQ(fb_frame_get_nb_samples(frame), 1024);
	EXPECT_EQ(fb_frame_get_sample_rate(frame), 44100);
	EXPECT_EQ(fb_frame_get_channel_layout_mask(frame), FB_CH_LAYOUT_STEREO);
	// Planar stereo: two data planes
	EXPECT_NE(fb_frame_get_data(frame, 0), nullptr);
	EXPECT_NE(fb_frame_get_data(frame, 1), nullptr);

	fb_frame_free(&frame);
}

TEST(FFmpegBridgeFrame, CopyProps)
{
	FBFrame *src = fb_frame_alloc();
	FBFrame *dst = fb_frame_alloc();
	ASSERT_NE(src, nullptr);
	ASSERT_NE(dst, nullptr);

	// av_frame_copy_props copies metadata properties, not dimensions/format
	fb_frame_set_pts(src, 777);
	fb_frame_set_color_range(src, FB_COLOR_RANGE_JPEG);
	fb_frame_set_colorspace(src, FB_COL_SPC_BT709);

	ASSERT_EQ(fb_frame_copy_props(dst, src), 0);
	EXPECT_EQ(fb_frame_get_pts(dst), 777);
	EXPECT_EQ(fb_frame_get_color_range(dst), FB_COLOR_RANGE_JPEG);
	EXPECT_EQ(fb_frame_get_colorspace(dst), FB_COL_SPC_BT709);

	fb_frame_free(&src);
	fb_frame_free(&dst);
}

TEST(FFmpegBridgeFrame, NullSafety)
{
	EXPECT_LT(fb_frame_get_buffer(nullptr, 0), 0);
	EXPECT_LT(fb_frame_make_writable(nullptr), 0);
	EXPECT_EQ(fb_frame_get_width(nullptr), 0);
	EXPECT_EQ(fb_frame_get_data(nullptr, 0), nullptr);
	fb_frame_free(nullptr); // must not crash
}

// ============================================================================
// Packet
// ============================================================================

TEST(FFmpegBridgePacket, AllocAndDefaults)
{
	FBPacket *packet = fb_packet_alloc();
	ASSERT_NE(packet, nullptr);

	EXPECT_EQ(fb_packet_get_pts(packet), FB_NOPTS_VALUE);
	EXPECT_EQ(fb_packet_get_size(packet), 0);
	EXPECT_EQ(fb_packet_get_data(packet), nullptr);

	fb_packet_unref(packet);
	fb_packet_free(&packet);
	EXPECT_EQ(packet, nullptr);
}

// ============================================================================
// Scaler
// ============================================================================

TEST(FFmpegBridgeScaler, RgbaToYuv420P)
{
	const int width = 64;
	const int height = 64;

	FBFrame *src = fb_frame_alloc();
	fb_frame_set_width(src, width);
	fb_frame_set_height(src, height);
	fb_frame_set_format(src, FB_PIX_FMT_RGBA);
	ASSERT_EQ(fb_frame_get_buffer(src, 0), 0);

	// Solid mid-grey image
	const int src_linesize = fb_frame_get_linesize(src, 0);
	for (int y = 0; y < height; ++y) {
		std::memset(fb_frame_get_data(src, 0) + y * src_linesize, 128,
					width * 4);
	}

	FBFrame *dst = fb_frame_alloc();
	fb_frame_set_width(dst, width);
	fb_frame_set_height(dst, height);
	fb_frame_set_format(dst, FB_PIX_FMT_YUV420P);
	ASSERT_EQ(fb_frame_get_buffer(dst, 0), 0);

	FBScaler *scaler = fb_scaler_create(width, height, FB_PIX_FMT_RGBA, width,
										height, FB_PIX_FMT_YUV420P,
										FB_SCALER_POINT);
	ASSERT_NE(scaler, nullptr);
	// sws_scale returns the output slice height on success
	ASSERT_GE(fb_scaler_scale_frame(scaler, dst, src), 0);
	fb_scaler_free(&scaler);
	EXPECT_EQ(scaler, nullptr);

	// Y plane of a mid-grey RGB source should sit near 128
	const uint8_t *y_plane = fb_frame_get_data_const(dst, 0);
	ASSERT_NE(y_plane, nullptr);
	EXPECT_NEAR(y_plane[0], 128, 8);

	// U and V planes of a grey image should sit near 128 (neutral chroma)
	const uint8_t *u_plane = fb_frame_get_data_const(dst, 1);
	const uint8_t *v_plane = fb_frame_get_data_const(dst, 2);
	ASSERT_NE(u_plane, nullptr);
	ASSERT_NE(v_plane, nullptr);
	EXPECT_NEAR(u_plane[0], 128, 8);
	EXPECT_NEAR(v_plane[0], 128, 8);

	fb_frame_free(&src);
	fb_frame_free(&dst);
}

TEST(FFmpegBridgeScaler, SetColorspace)
{
	FBScaler *scaler = fb_scaler_create(64, 64, FB_PIX_FMT_YUV420P, 64, 64,
										FB_PIX_FMT_RGBA, FB_SCALER_POINT);
	ASSERT_NE(scaler, nullptr);
	EXPECT_GE(fb_scaler_set_colorspace(scaler, FB_COL_SPC_BT709, 0), 0);
	fb_scaler_free(&scaler);
}

TEST(FFmpegBridgeScaler, YuvCoefficients)
{
	// Values come from swscale's coefficient tables (sws_getCoefficients /
	// 65536), which include the studio-range scaling factor.
	double coeffs[4] = { 0, 0, 0, 0 };
	fb_get_yuv_coefficients(FB_COL_SPC_BT709, coeffs);
	EXPECT_NEAR(coeffs[0], 117489 / 65536.0, 1e-6); // crv
	EXPECT_NEAR(coeffs[1], 138438 / 65536.0, 1e-6); // cbu
	EXPECT_GT(coeffs[2], 0.0);
	EXPECT_GT(coeffs[3], 0.0);

	fb_get_yuv_coefficients(FB_COL_SPC_SMPTE170M, coeffs);
	EXPECT_NEAR(coeffs[0], 104597 / 65536.0, 1e-6); // crv
}

// ============================================================================
// Resampler
// ============================================================================

TEST(FFmpegBridgeResampler, ConvertFltpToS16p)
{
	const int in_samples = 1024;
	const double freq = 440.0;
	const double rate = 44100.0;

	FBResampler *resampler =
		fb_resampler_create(FB_CH_LAYOUT_STEREO, FB_SAMPLE_FMT_S16P, 44100,
							FB_CH_LAYOUT_STEREO, FB_SAMPLE_FMT_FLTP, 44100);
	ASSERT_NE(resampler, nullptr);

	const int out_capacity = fb_resampler_get_out_samples(resampler, in_samples);
	ASSERT_GT(out_capacity, 0);

	std::vector<float> in_left(in_samples), in_right(in_samples);
	for (int i = 0; i < in_samples; ++i) {
		in_left[i] = 0.5f * std::sin(2.0 * kPi * freq * i / rate);
		in_right[i] = 0.5f * std::sin(2.0 * kPi * freq * i / rate);
	}
	const uint8_t *in_planes[2] = {
		reinterpret_cast<const uint8_t *>(in_left.data()),
		reinterpret_cast<const uint8_t *>(in_right.data())
	};

	std::vector<int16_t> out_left(out_capacity), out_right(out_capacity);
	uint8_t *out_planes[2] = {
		reinterpret_cast<uint8_t *>(out_left.data()),
		reinterpret_cast<uint8_t *>(out_right.data())
	};

	const int converted = fb_resampler_convert(
		resampler, out_planes, out_capacity, in_planes, in_samples);
	ASSERT_GT(converted, 0);

	// Output must be non-silent and bounded to full scale
	bool non_silent = false;
	for (int i = 0; i < converted; ++i) {
		if (out_left[i] != 0) {
			non_silent = true;
		}
		EXPECT_LE(std::abs(out_left[i]), 32767);
	}
	EXPECT_TRUE(non_silent);

	fb_resampler_free(&resampler);
	EXPECT_EQ(resampler, nullptr);
}

TEST(FFmpegBridgeResampler, ConvertFrameInput)
{
	const int in_samples = 512;

	FBResampler *resampler =
		fb_resampler_create(FB_CH_LAYOUT_STEREO, FB_SAMPLE_FMT_S16P, 44100,
							FB_CH_LAYOUT_STEREO, FB_SAMPLE_FMT_FLTP, 44100);
	ASSERT_NE(resampler, nullptr);

	FBFrame *frame = fb_frame_alloc();
	fb_frame_set_nb_samples(frame, in_samples);
	fb_frame_set_sample_rate(frame, 44100);
	fb_frame_set_format(frame, FB_SAMPLE_FMT_FLTP);
	fb_frame_set_channel_layout_mask(frame, FB_CH_LAYOUT_STEREO);
	ASSERT_EQ(fb_frame_get_buffer(frame, 0), 0);

	float *left = reinterpret_cast<float *>(fb_frame_get_data(frame, 0));
	float *right = reinterpret_cast<float *>(fb_frame_get_data(frame, 1));
	for (int i = 0; i < in_samples; ++i) {
		left[i] = 0.25f;
		right[i] = -0.25f;
	}

	const int out_capacity = fb_resampler_get_out_samples(resampler, in_samples);
	std::vector<int16_t> out_left(out_capacity), out_right(out_capacity);
	uint8_t *out_planes[2] = {
		reinterpret_cast<uint8_t *>(out_left.data()),
		reinterpret_cast<uint8_t *>(out_right.data())
	};

	const int converted =
		fb_resampler_convert_frame(resampler, out_planes, out_capacity, frame);
	ASSERT_GT(converted, 0);

	// Constant 0.25 input should land near 0.25 * 32767
	EXPECT_NEAR(out_left[converted / 2], 8192, 256);
	EXPECT_NEAR(out_right[converted / 2], -8192, 256);

	fb_frame_free(&frame);
	fb_resampler_free(&resampler);
}

// ============================================================================
// Probe / Decoder on tests/demo.mp4
// ============================================================================

TEST(FFmpegBridgeProbe, DemoMp4Streams)
{
	const QString path = DemoPath();
	ASSERT_TRUE(QFileInfo::exists(path));

	FBProbe *probe = fb_probe_create();
	ASSERT_NE(probe, nullptr);
	ASSERT_EQ(fb_probe_open(probe, path.toUtf8().constData()), 0);

	EXPECT_GE(fb_probe_get_stream_count(probe), 1);
	EXPECT_GT(fb_probe_get_duration(probe), 0);

	const int video = FindStream(probe, FB_MEDIA_TYPE_VIDEO);
	ASSERT_GE(video, 0);

	FBStreamInfo info;
	ASSERT_EQ(fb_probe_get_stream_info(probe, video, &info), 0);
	EXPECT_EQ(info.width, 1920);
	EXPECT_EQ(info.height, 1080);
	EXPECT_NE(info.pixel_format, FB_PIX_FMT_NONE);
	EXPECT_EQ(info.has_decoder, 1);
	EXPECT_GT(info.time_base_den, 0);

	// Metadata lookups must be safe whether or not the key exists
	char buffer[256];
	int found = fb_probe_get_metadata(probe, -1, "title", buffer, sizeof(buffer));
	EXPECT_TRUE(found == 0 || found == 1);

	fb_probe_close(probe);
	fb_probe_free(&probe);
	EXPECT_EQ(probe, nullptr);
}

TEST(FFmpegBridgeProbe, VideoStreamDetails)
{
	const QString path = DemoPath();
	ASSERT_TRUE(QFileInfo::exists(path));

	FBVideoStreamDetails details;
	ASSERT_EQ(fb_probe_video_stream_details(path.toUtf8().constData(), 0,
											&details, 0, nullptr, nullptr),
			  0);
	EXPECT_TRUE(details.field_order == FB_FIELD_ORDER_PROGRESSIVE ||
				details.field_order == FB_FIELD_ORDER_UNKNOWN);
	EXPECT_GT(details.frame_rate_num, 0);
	EXPECT_GT(details.frame_rate_den, 0);
	EXPECT_EQ(details.pixel_aspect_num, 1);
	EXPECT_EQ(details.pixel_aspect_den, 1);
}

TEST(FFmpegBridgeProbe, ReadSubtitleStream)
{
	// Write a small SRT file and read it back through the bridge
	const QString path = TempFilePath(QStringLiteral("fb_bridge_test.srt"));
	{
		QFile file(path);
		ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
		file.write("1\n00:00:01,000 --> 00:00:02,000\nHello bridge\n\n"
				   "2\n00:00:03,000 --> 00:00:04,500\nSecond line\n\n");
		file.close();
	}

	struct Context {
		int count = 0;
		std::string first_text;
		int64_t first_pts = FB_NOPTS_VALUE;
	} ctx;

	int result = fb_probe_read_subtitle_stream(
		path.toUtf8().constData(), 0,
		[](int64_t pts, int64_t, const char *text, int, void *userdata) {
			auto *c = static_cast<Context *>(userdata);
			if (c->count == 0) {
				c->first_pts = pts;
				c->first_text = text ? text : "";
			}
			c->count++;
		},
		&ctx);

	EXPECT_EQ(result, 0);
	EXPECT_EQ(ctx.count, 2);
	EXPECT_NE(ctx.first_text.find("Hello bridge"), std::string::npos);
	EXPECT_GE(ctx.first_pts, 0);

	QFile::remove(path);
}

TEST(FFmpegBridgeDecoder, DecodeFirstFrame)
{
	const QString path = DemoPath();
	ASSERT_TRUE(QFileInfo::exists(path));

	FBDecoder *decoder = fb_decoder_create();
	ASSERT_NE(decoder, nullptr);
	ASSERT_EQ(fb_decoder_open(decoder, path.toUtf8().constData(), 0), 0);

	FBStreamInfo info;
	ASSERT_EQ(fb_decoder_get_stream_info(decoder, &info), 0);
	EXPECT_EQ(info.width, 1920);
	EXPECT_EQ(info.height, 1080);
	EXPECT_EQ(info.codec_type, FB_MEDIA_TYPE_VIDEO);
	EXPECT_GT(fb_decoder_get_format_duration(decoder), 0);

	FBPacket *packet = fb_packet_alloc();
	FBFrame *frame = fb_frame_alloc();
	ASSERT_NE(packet, nullptr);
	ASSERT_NE(frame, nullptr);

	int result = -1;
	for (int attempt = 0; attempt < 200 && result != 0; ++attempt) {
		result = fb_decoder_get_frame(decoder, packet, frame);
	}
	ASSERT_EQ(result, 0);

	EXPECT_EQ(fb_frame_get_width(frame), 1920);
	EXPECT_EQ(fb_frame_get_height(frame), 1080);
	EXPECT_NE(fb_frame_get_format(frame), FB_PIX_FMT_NONE);
	// Software frames must have CPU-accessible data. Hardware frames live in
	// device memory, so their data is checked after the transfer below.
	if (!fb_frame_is_hw(frame)) {
		EXPECT_NE(fb_frame_get_data(frame, 0), nullptr);
	}

	// Hardware frames live in device memory: transfer to a software frame
	// before reading pixels
	FBFrame *sw_frame = nullptr;
	const FBFrame *read_frame = frame;
	if (fb_frame_is_hw(frame)) {
		sw_frame = fb_frame_alloc();
		ASSERT_NE(sw_frame, nullptr);
		ASSERT_EQ(fb_frame_hw_transfer_data(sw_frame, frame), 0);
		read_frame = sw_frame;
	}

	// The decoded image should not be a black frame
	const uint8_t *data = fb_frame_get_data_const(read_frame, 0);
	ASSERT_NE(data, nullptr);
	bool has_nonzero = false;
	const int linesize = fb_frame_get_linesize(read_frame, 0);
	for (int y = 0; y < 64 && !has_nonzero; ++y) {
		for (int x = 0; x < 64; ++x) {
			if (data[y * linesize + x] != 0) {
				has_nonzero = true;
				break;
			}
		}
	}
	EXPECT_TRUE(has_nonzero);

	fb_frame_free(&sw_frame);

	// Seek back to the start and decode again
	fb_decoder_seek(decoder, 0);
	result = -1;
	for (int attempt = 0; attempt < 200 && result != 0; ++attempt) {
		result = fb_decoder_get_frame(decoder, packet, frame);
	}
	EXPECT_EQ(result, 0);

	fb_frame_free(&frame);
	fb_packet_free(&packet);
	fb_decoder_close(decoder);
	fb_decoder_free(&decoder);
}

TEST(FFmpegBridgeDecoder, OpenFailure)
{
	FBDecoder *decoder = fb_decoder_create();
	ASSERT_NE(decoder, nullptr);
	EXPECT_LT(fb_decoder_open(decoder, "/nonexistent/file.mp4", 0), 0);
	fb_decoder_free(&decoder);
}

TEST(FFmpegBridgeDecoder, GuessRates)
{
	const QString path = DemoPath();
	ASSERT_TRUE(QFileInfo::exists(path));

	FBDecoder *decoder = fb_decoder_create();
	ASSERT_EQ(fb_decoder_open(decoder, path.toUtf8().constData(), 0), 0);

	int num = 0, den = 0;
	ASSERT_EQ(fb_decoder_guess_frame_rate(decoder, nullptr, &num, &den), 0);
	EXPECT_GT(num, 0);
	EXPECT_GT(den, 0);

	ASSERT_EQ(fb_decoder_guess_sample_aspect_ratio(decoder, nullptr, &num, &den),
			  0);
	EXPECT_EQ(num, 1);
	EXPECT_EQ(den, 1);

	fb_decoder_free(&decoder);
}

// ============================================================================
// Audio graph
// ============================================================================

TEST(FFmpegBridgeAudioGraph, TempoProcessing)
{
	FBAudioGraphConfig config = {};
	config.in_sample_rate = 44100;
	config.in_channel_layout_mask = FB_CH_LAYOUT_STEREO;
	config.in_sample_format = FB_SAMPLE_FMT_FLTP;
	config.in_channels = 2;
	config.out_sample_rate = 44100;
	config.out_channel_layout_mask = FB_CH_LAYOUT_STEREO;
	config.out_sample_format = FB_SAMPLE_FMT_FLTP;
	config.out_channels = 2;
	config.out_is_planar = 1;
	config.tempo = 2.0;

	FBAudioGraph *graph = fb_audio_graph_create(&config);
	ASSERT_NE(graph, nullptr);

	const int in_samples = 4410; // 0.1 seconds
	std::vector<float> left(in_samples, 0.5f), right(in_samples, 0.5f);
	const uint8_t *planes[2] = {
		reinterpret_cast<const uint8_t *>(left.data()),
		reinterpret_cast<const uint8_t *>(right.data())
	};

	ASSERT_EQ(fb_audio_graph_push(graph, planes, in_samples), 0);
	// Flush
	ASSERT_EQ(fb_audio_graph_push(graph, nullptr, 0), 0);

	FBFrame *out = fb_frame_alloc();
	int total_samples = 0;
	int pull_result;
	while ((pull_result = fb_audio_graph_pull(graph, out)) == 1) {
		total_samples += fb_frame_get_nb_samples(out);
		fb_frame_unref(out);
	}
	// 0 = needs more input, FB_ERROR_EOF = flushed graph is drained
	EXPECT_TRUE(pull_result == 0 || pull_result == FB_ERROR_EOF);

	// tempo 2.0 halves the duration; atempo has some internal padding,
	// so allow generous bounds around the ideal 2205 samples
	EXPECT_GT(total_samples, 1500);
	EXPECT_LT(total_samples, 3000);

	fb_frame_free(&out);
	fb_audio_graph_free(&graph);
	EXPECT_EQ(graph, nullptr);
}

// ============================================================================
// Encoder (end-to-end: encode, then probe the result back)
// ============================================================================

TEST(FFmpegBridgeEncoder, CodecFormatLists)
{
	// PNG is a native FFmpeg encoder and always available
	const char *names[16];
	const int pix_count =
		fb_encoder_codec_get_pixel_formats(FB_CODEC_PNG, names, 16);
	EXPECT_GT(pix_count, 0);

	int fmts[16];
	const int sample_count =
		fb_encoder_codec_get_sample_formats(FB_CODEC_AAC, fmts, 16);
	EXPECT_GT(sample_count, 0);
}

TEST(FFmpegBridgeEncoder, WritePngVideoAndProbeBack)
{
	const QString path = TempFilePath(QStringLiteral("fb_bridge_test.mkv"));
	QFile::remove(path);

	const int width = 64;
	const int height = 64;
	const int frame_count = 5;

	const QByteArray filename = path.toUtf8();

	FBEncoderConfig config = {};
	config.filename = filename.constData();
	config.video_enabled = 1;
	config.video_codec = FB_CODEC_PNG;
	config.video_width = width;
	config.video_height = height;
	config.video_pixel_aspect_num = 1;
	config.video_pixel_aspect_den = 1;
	config.video_time_base_num = 1;
	config.video_time_base_den = 30;
	config.video_frame_rate_num = 30;
	config.video_frame_rate_den = 1;
	config.video_pix_fmt = "rgba";
	config.video_src_pix_fmt = FB_PIX_FMT_RGBA;
	config.video_color_range = FB_COLOR_RANGE_UNSPEC;
	config.video_field_order = FB_FIELD_ORDER_PROGRESSIVE;
	config.video_threads = 1;

	FBEncoder *encoder = fb_encoder_create(&config);
	ASSERT_NE(encoder, nullptr);
	ASSERT_EQ(fb_encoder_open(encoder), 0)
		<< "encoder error: " << fb_encoder_get_error(encoder);

	std::vector<uint8_t> pixels(width * height * 4);
	for (int f = 0; f < frame_count; ++f) {
		for (int i = 0; i < width * height; ++i) {
			pixels[i * 4 + 0] = static_cast<uint8_t>(f * 40); // R
			pixels[i * 4 + 1] = 128;							// G
			pixels[i * 4 + 2] = 64;								// B
			pixels[i * 4 + 3] = 255;							// A
		}
		ASSERT_EQ(fb_encoder_write_video_frame(encoder, width, height,
											 FB_PIX_FMT_RGBA, pixels.data(),
											 width * 4, f / 30.0),
				  0)
			<< "encoder error: " << fb_encoder_get_error(encoder);
	}

	fb_encoder_close(encoder);
	fb_encoder_free(&encoder);

	// The file must exist and probe back as a 64x64 video
	ASSERT_TRUE(QFileInfo::exists(path));

	FBProbe *probe = fb_probe_create();
	ASSERT_EQ(fb_probe_open(probe, path.toUtf8().constData()), 0);
	const int video = FindStream(probe, FB_MEDIA_TYPE_VIDEO);
	ASSERT_GE(video, 0);

	FBStreamInfo info;
	ASSERT_EQ(fb_probe_get_stream_info(probe, video, &info), 0);
	EXPECT_EQ(info.width, width);
	EXPECT_EQ(info.height, height);

	fb_probe_close(probe);
	fb_probe_free(&probe);
	QFile::remove(path);
}

TEST(FFmpegBridgeEncoder, WritePcmAudioAndProbeBack)
{
	const QString path = TempFilePath(QStringLiteral("fb_bridge_test.wav"));
	QFile::remove(path);

	const QByteArray filename = path.toUtf8();

	FBEncoderConfig config = {};
	config.filename = filename.constData();
	config.audio_enabled = 1;
	config.audio_codec = FB_CODEC_PCM;
	config.audio_sample_rate = 44100;
	config.audio_channel_layout_mask = FB_CH_LAYOUT_STEREO;
	config.audio_sample_format = FB_SAMPLE_FMT_S16;

	FBEncoder *encoder = fb_encoder_create(&config);
	ASSERT_NE(encoder, nullptr);
	ASSERT_EQ(fb_encoder_open(encoder), 0)
		<< "encoder error: " << fb_encoder_get_error(encoder);

	const int sample_count = 4410;
	std::vector<int16_t> left(sample_count), right(sample_count);
	for (int i = 0; i < sample_count; ++i) {
		left[i] = static_cast<int16_t>(
			10000 * std::sin(2.0 * kPi * 440.0 * i / 44100.0));
		right[i] = left[i];
	}
	const uint8_t *planes[2] = {
		reinterpret_cast<const uint8_t *>(left.data()),
		reinterpret_cast<const uint8_t *>(right.data())
	};

	ASSERT_EQ(fb_encoder_write_audio(encoder, planes, 2, FB_SAMPLE_FMT_S16P,
									 44100, FB_CH_LAYOUT_STEREO, sample_count),
			  0)
		<< "encoder error: " << fb_encoder_get_error(encoder);

	// Flush
	EXPECT_EQ(fb_encoder_write_audio(encoder, nullptr, 2, FB_SAMPLE_FMT_S16P,
									 44100, FB_CH_LAYOUT_STEREO, 0),
			  0);

	fb_encoder_close(encoder);
	fb_encoder_free(&encoder);

	ASSERT_TRUE(QFileInfo::exists(path));

	FBProbe *probe = fb_probe_create();
	ASSERT_EQ(fb_probe_open(probe, path.toUtf8().constData()), 0);
	const int audio = FindStream(probe, FB_MEDIA_TYPE_AUDIO);
	ASSERT_GE(audio, 0);

	FBStreamInfo info;
	ASSERT_EQ(fb_probe_get_stream_info(probe, audio, &info), 0);
	EXPECT_EQ(info.sample_rate, 44100);
	EXPECT_EQ(fb_channel_layout_get_channels(info.channel_layout_mask), 2);

	fb_probe_close(probe);
	fb_probe_free(&probe);
	QFile::remove(path);
}
