#include <gtest/gtest.h>

#include "codec/encoder.h"

namespace
{
class TestEncoder final : public olive::Encoder {
public:
	explicit TestEncoder(const olive::EncodingParams &params)
		: olive::Encoder(params)
	{
	}

	bool Open() override
	{
		return true;
	}

	bool WriteFrame(olive::FramePtr, olive::core::rational) override
	{
		return true;
	}

	bool WriteAudio(const olive::SampleBuffer &) override
	{
		return true;
	}

	bool WriteSubtitle(const olive::SubtitleBlock *) override
	{
		return true;
	}

	void Close() override
	{
	}
};
}

TEST(CodecEncoder, ImageSequenceFilenames)
{
	olive::EncodingParams params;
	params.SetFilename(QStringLiteral("frame_[####].png"));
	params.set_video_is_image_sequence(true);

	olive::VideoParams video_params;
	video_params.set_frame_rate(olive::core::rational(24, 1));
	params.EnableVideo(video_params, olive::ExportCodec::kCodecPNG);

	TestEncoder encoder(params);

	EXPECT_TRUE(olive::Encoder::FilenameContainsDigitPlaceholder(
		QStringLiteral("frame_[####].png")));
	EXPECT_EQ(olive::Encoder::GetImageSequencePlaceholderDigitCount(
				  QStringLiteral("frame_[####].png")),
			  4);
	EXPECT_EQ(olive::Encoder::FilenameRemoveDigitPlaceholder(
				  QStringLiteral("frame_[####].png")),
			  QStringLiteral("frame.png"));

	const QString filename =
		encoder.GetFilenameForFrame(olive::core::rational(1, 24));
	EXPECT_EQ(filename, QStringLiteral("frame_0001.png"));
}

TEST(CodecEncoder, MatrixGeneration)
{
	using Method = olive::EncodingParams::VideoScalingMethod;

	QMatrix4x4 stretch = olive::EncodingParams::GenerateMatrix(
		Method::kStretch, 1920, 1080, 1280, 720);
	EXPECT_TRUE(qFuzzyCompare(stretch(0, 0), 1.0f));
	EXPECT_TRUE(qFuzzyCompare(stretch(1, 1), 1.0f));

	QMatrix4x4 fit = olive::EncodingParams::GenerateMatrix(Method::kFit, 1920,
														   1080, 1024, 1024);
	EXPECT_TRUE(qFuzzyCompare(fit(0, 0), 1.0f));
	EXPECT_FALSE(qFuzzyCompare(fit(1, 1), 1.0f));

	QMatrix4x4 crop = olive::EncodingParams::GenerateMatrix(Method::kCrop, 1920,
															1080, 1024, 1024);
	EXPECT_FALSE(qFuzzyCompare(crop(0, 0), 1.0f));
	EXPECT_TRUE(qFuzzyCompare(crop(1, 1), 1.0f));
}

TEST(CodecEncoder, TypeFromFormat)
{
	using olive::Encoder;
	using olive::ExportFormat;

	EXPECT_EQ(Encoder::GetTypeFromFormat(ExportFormat::kFormatPNG),
			  Encoder::kEncoderTypeOIIO);
	EXPECT_EQ(Encoder::GetTypeFromFormat(ExportFormat::kFormatDNxHD),
			  Encoder::kEncoderTypeFFmpeg);
	EXPECT_EQ(Encoder::GetTypeFromFormat(ExportFormat::kFormatCount),
			  Encoder::kEncoderTypeNone);
}
