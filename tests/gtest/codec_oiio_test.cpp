#include <gtest/gtest.h>

#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>

#include "codec/decoder.h"
#include "codec/oiio/oiiodecoder.h"
#include "codec/oiio/oiioencoder.h"
#include "node/project/footage/footagedescription.h"

namespace
{
QString ImgPath()
{
	return QDir(QStringLiteral(OAK_TEST_SOURCE_DIR))
		.filePath(QStringLiteral("tests/img.png"));
}
}

TEST(CodecOIIO, ProbePngReportsStillImage)
{
	olive::OIIODecoder decoder;
	olive::FootageDescription desc = decoder.Probe(ImgPath(), nullptr);

	ASSERT_TRUE(desc.IsValid());
	EXPECT_EQ(desc.decoder(), QStringLiteral("oiio"));
	EXPECT_EQ(desc.GetStreamCount(), 1);

	const QVector<olive::VideoParams> &streams = desc.GetVideoStreams();
	ASSERT_EQ(streams.size(), 1);
	EXPECT_EQ(streams.first().width(), 1920);
	EXPECT_EQ(streams.first().height(), 1080);
	EXPECT_EQ(streams.first().format(), olive::core::PixelFormat::U8);
	EXPECT_EQ(streams.first().channel_count(), 4);
	EXPECT_EQ(streams.first().video_type(), olive::VideoParams::kVideoTypeStill);
	EXPECT_EQ(streams.first().stream_index(), 0);
	EXPECT_TRUE(streams.first().enabled());
}

TEST(CodecOIIO, ProbeUnsupportedExtensionReturnsInvalid)
{
	olive::OIIODecoder decoder;
	olive::FootageDescription desc =
		decoder.Probe(QStringLiteral("nonexistent.zzz"), nullptr);

	EXPECT_FALSE(desc.IsValid());
	EXPECT_TRUE(desc.GetVideoStreams().isEmpty());
}

TEST(CodecOIIO, DecodePngFrame)
{
	const QString path = ImgPath();
	ASSERT_TRUE(QFileInfo::exists(path));

	olive::DecoderPtr decoder =
		olive::Decoder::CreateFromID(QStringLiteral("oiio"));
	ASSERT_TRUE(decoder);

	ASSERT_TRUE(decoder->Open(olive::Decoder::CodecStream(path, 0, nullptr)));

	olive::Decoder::RetrieveVideoParams params;
	params.time = olive::core::rational(0);
	params.divider = 1;

	olive::FramePtr frame = decoder->RetrieveVideoFrame(params);
	ASSERT_TRUE(frame);
	ASSERT_TRUE(frame->is_allocated());
	EXPECT_EQ(frame->width(), 1920);
	EXPECT_EQ(frame->height(), 1080);
	// Still images are always converted to F32 by OIIODecoder
	EXPECT_EQ(frame->format(), olive::core::PixelFormat::F32);
	EXPECT_EQ(frame->channel_count(), 4);
	EXPECT_EQ(frame->timestamp(), olive::core::rational(0));
	EXPECT_GT(frame->allocated_size(), 0);

	bool has_nonzero_byte = false;
	const char *data = frame->const_data();
	for (int i = 0; i < frame->allocated_size(); i++) {
		if (data[i] != 0) {
			has_nonzero_byte = true;
			break;
		}
	}
	EXPECT_TRUE(has_nonzero_byte);

	decoder->Close();
}

TEST(CodecOIIO, DecodeWithDividerHalvesResolution)
{
	olive::DecoderPtr decoder =
		olive::Decoder::CreateFromID(QStringLiteral("oiio"));
	ASSERT_TRUE(decoder);

	ASSERT_TRUE(
		decoder->Open(olive::Decoder::CodecStream(ImgPath(), 0, nullptr)));

	olive::Decoder::RetrieveVideoParams params;
	params.time = olive::core::rational(0);
	params.divider = 2;

	olive::FramePtr frame = decoder->RetrieveVideoFrame(params);
	ASSERT_TRUE(frame);
	ASSERT_TRUE(frame->is_allocated());
	EXPECT_EQ(frame->width(), 960);
	EXPECT_EQ(frame->height(), 540);

	decoder->Close();
}

TEST(CodecOIIO, EncodePngAndDecodeBack)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString path = dir.filePath(QStringLiteral("roundtrip.png"));

	const int width = 64;
	const int height = 32;

	olive::EncodingParams params;
	params.SetFilename(path);
	params.EnableVideo(
		olive::VideoParams(width, height, olive::core::PixelFormat::U8,
						   olive::VideoParams::kRGBAChannelCount),
		olive::ExportCodec::kCodecPNG);

	olive::OIIOEncoder encoder(params);
	ASSERT_TRUE(encoder.Open());

	olive::FramePtr frame = olive::Frame::Create();
	frame->set_video_params(params.video_params());
	ASSERT_TRUE(frame->allocate());
	for (int y = 0; y < height; y++) {
		uint8_t *row = reinterpret_cast<uint8_t *>(frame->data()) +
					   y * frame->linesize_bytes();
		for (int x = 0; x < width; x++) {
			row[x * 4 + 0] = uint8_t(x * 2);
			row[x * 4 + 1] = uint8_t(y * 4);
			row[x * 4 + 2] = 200;
			row[x * 4 + 3] = 255;
		}
	}

	EXPECT_TRUE(encoder.WriteFrame(frame, olive::core::rational(0)));
	encoder.Close();

	ASSERT_TRUE(QFileInfo::exists(path));

	// Decode the file we just wrote and verify dimensions and pixels
	olive::DecoderPtr decoder =
		olive::Decoder::CreateFromID(QStringLiteral("oiio"));
	ASSERT_TRUE(decoder);
	ASSERT_TRUE(decoder->Open(olive::Decoder::CodecStream(path, 0, nullptr)));

	olive::Decoder::RetrieveVideoParams rp;
	rp.time = olive::core::rational(0);
	rp.divider = 1;

	olive::FramePtr back = decoder->RetrieveVideoFrame(rp);
	ASSERT_TRUE(back);
	EXPECT_EQ(back->width(), width);
	EXPECT_EQ(back->height(), height);

	// U8 -> F32 conversion scales by 1/255
	const float eps = 1.0f / 255.0f;
	olive::core::Color c = back->get_pixel(10, 5);
	EXPECT_NEAR(c.red(), 20.0f / 255.0f, eps);
	EXPECT_NEAR(c.green(), 20.0f / 255.0f, eps);
	EXPECT_NEAR(c.blue(), 200.0f / 255.0f, eps);
	EXPECT_NEAR(c.alpha(), 1.0f, eps);

	decoder->Close();
}

TEST(CodecOIIO, EncoderRejectsAudioAndSubtitles)
{
	olive::EncodingParams params;
	olive::OIIOEncoder encoder(params);

	olive::SampleBuffer buffer;
	EXPECT_FALSE(encoder.WriteAudio(buffer));
	EXPECT_FALSE(encoder.WriteSubtitle(nullptr));
}
