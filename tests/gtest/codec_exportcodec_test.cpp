#include <gtest/gtest.h>

#include "codec/exportcodec.h"

TEST(CodecExportCodec, NamesAndFlags)
{
	using olive::ExportCodec;

	EXPECT_EQ(ExportCodec::GetCodecName(ExportCodec::kCodecH264),
			  QStringLiteral("H.264"));
	EXPECT_EQ(ExportCodec::GetCodecName(ExportCodec::kCodecCount),
			  QStringLiteral("Unknown"));

	EXPECT_TRUE(ExportCodec::IsCodecAStillImage(ExportCodec::kCodecPNG));
	EXPECT_FALSE(ExportCodec::IsCodecAStillImage(ExportCodec::kCodecH264));

	EXPECT_TRUE(ExportCodec::IsCodecLossless(ExportCodec::kCodecPCM));
	EXPECT_TRUE(ExportCodec::IsCodecLossless(ExportCodec::kCodecFLAC));
	EXPECT_FALSE(ExportCodec::IsCodecLossless(ExportCodec::kCodecH265));
}

TEST(CodecExportCodec, VideoCodecNamesAreNonEmpty)
{
	using olive::ExportCodec;

	for (int i = 0; i < ExportCodec::kCodecCount; ++i) {
		const auto codec = static_cast<ExportCodec::Codec>(i);
		const QString name = ExportCodec::GetCodecName(codec);
		EXPECT_FALSE(name.isEmpty()) << "Codec " << i;
	}
}

TEST(CodecExportCodec, AudioCodecsAreNotStillImages)
{
	using olive::ExportCodec;

	EXPECT_FALSE(ExportCodec::IsCodecAStillImage(ExportCodec::kCodecPCM));
	EXPECT_FALSE(ExportCodec::IsCodecAStillImage(ExportCodec::kCodecAAC));
	EXPECT_FALSE(ExportCodec::IsCodecAStillImage(ExportCodec::kCodecFLAC));
	EXPECT_FALSE(ExportCodec::IsCodecAStillImage(ExportCodec::kCodecOpus));
}

TEST(CodecExportCodec, LossyCodecsAreNotLossless)
{
	using olive::ExportCodec;

	EXPECT_FALSE(ExportCodec::IsCodecLossless(ExportCodec::kCodecH264));
	EXPECT_FALSE(ExportCodec::IsCodecLossless(ExportCodec::kCodecH265));
	EXPECT_FALSE(ExportCodec::IsCodecLossless(ExportCodec::kCodecVP9));
	EXPECT_FALSE(ExportCodec::IsCodecLossless(ExportCodec::kCodecAAC));
}
