#include <gtest/gtest.h>

#include <vector>

#include <QFileInfo>
#include <QTemporaryDir>

#include <ffmpeg_bridge/ffmpeg_bridge.h>

#include "codec/ffmpeg/ffmpegencoder.h"

TEST(CodecFFmpegEncoder, PixelFormatsForCodec)
{
	olive::EncodingParams params;
	olive::FFmpegEncoder encoder(params);

	const QStringList png_fmts =
		encoder.GetPixelFormatsForCodec(olive::ExportCodec::kCodecPNG);
	EXPECT_FALSE(png_fmts.isEmpty());
	EXPECT_TRUE(png_fmts.contains(QStringLiteral("rgba")));

	EXPECT_TRUE(
		encoder.GetPixelFormatsForCodec(olive::ExportCodec::kCodecCount)
			.isEmpty());
}

TEST(CodecFFmpegEncoder, SampleFormatsForCodec)
{
	olive::EncodingParams params;
	olive::FFmpegEncoder encoder(params);

	// PCM is handled with a custom list whose first element is the default
	const std::vector<olive::core::SampleFormat> pcm =
		encoder.GetSampleFormatsForCodec(olive::ExportCodec::kCodecPCM);
	ASSERT_FALSE(pcm.empty());
	EXPECT_EQ(pcm.front(), olive::core::SampleFormat::S16);

	EXPECT_FALSE(encoder.GetSampleFormatsForCodec(olive::ExportCodec::kCodecAAC)
					 .empty());
	EXPECT_TRUE(encoder.GetSampleFormatsForCodec(olive::ExportCodec::kCodecCount)
					.empty());
}

TEST(CodecFFmpegEncoder, OpenWithInvalidPixelFormatFails)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	olive::EncodingParams params;
	params.SetFilename(dir.filePath(QStringLiteral("invalid.mkv")));
	// A default-constructed VideoParams has PixelFormat::INVALID, for which no
	// bridge pixel format exists
	params.EnableVideo(olive::VideoParams(), olive::ExportCodec::kCodecPNG);

	olive::FFmpegEncoder encoder(params);
	EXPECT_FALSE(encoder.Open());
	EXPECT_FALSE(encoder.GetError().isEmpty());
}

TEST(CodecFFmpegEncoder, EncodePngVideoAndProbeBack)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString path = dir.filePath(QStringLiteral("encoder_test.mkv"));

	const int width = 64;
	const int height = 64;
	const int frame_count = 5;

	olive::EncodingParams params;
	params.SetFilename(path);
	olive::VideoParams video_params(width, height, olive::core::PixelFormat::U8,
									olive::VideoParams::kRGBAChannelCount);
	video_params.set_frame_rate(olive::core::rational(30, 1));
	params.EnableVideo(video_params, olive::ExportCodec::kCodecPNG);
	params.set_video_pix_fmt(QStringLiteral("rgba"));

	olive::FFmpegEncoder encoder(params);
	ASSERT_TRUE(encoder.Open()) << encoder.GetError().toStdString();
	EXPECT_EQ(encoder.GetDesiredPixelFormat(), olive::core::PixelFormat::U8);

	for (int f = 0; f < frame_count; f++) {
		olive::FramePtr frame = olive::Frame::Create();
		frame->set_video_params(video_params);
		ASSERT_TRUE(frame->allocate());
		for (int i = 0; i < width * height; i++) {
			uint8_t *px = reinterpret_cast<uint8_t *>(frame->data()) + i * 4;
			px[0] = uint8_t(f * 40);
			px[1] = 128;
			px[2] = 64;
			px[3] = 255;
		}
		ASSERT_TRUE(encoder.WriteFrame(frame, olive::core::rational(f, 30)))
			<< encoder.GetError().toStdString();
	}
	encoder.Close();

	// The file must exist and probe back as a 64x64 video
	ASSERT_TRUE(QFileInfo::exists(path));

	FBProbe *probe = fb_probe_create();
	ASSERT_NE(probe, nullptr);
	ASSERT_EQ(fb_probe_open(probe, path.toUtf8().constData()), 0);

	bool found_video = false;
	const int stream_count = fb_probe_get_stream_count(probe);
	for (int i = 0; i < stream_count; i++) {
		FBStreamInfo info;
		if (fb_probe_get_stream_info(probe, i, &info) == 0 &&
			info.codec_type == FB_MEDIA_TYPE_VIDEO) {
			found_video = true;
			EXPECT_EQ(info.width, width);
			EXPECT_EQ(info.height, height);
		}
	}
	EXPECT_TRUE(found_video);

	fb_probe_close(probe);
	fb_probe_free(&probe);
}
