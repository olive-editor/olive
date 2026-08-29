#include <gtest/gtest.h>

#include "codec/exportformat.h"

TEST(CodecExportFormat, NamesAndExtensions)
{
	using olive::ExportFormat;

	EXPECT_EQ(ExportFormat::GetName(ExportFormat::kFormatDNxHD),
			  QStringLiteral("DNxHD"));
	EXPECT_EQ(ExportFormat::GetExtension(ExportFormat::kFormatDNxHD),
			  QStringLiteral("mxf"));
	EXPECT_EQ(ExportFormat::GetName(ExportFormat::kFormatCount),
			  QStringLiteral("Unknown"));
	EXPECT_TRUE(
		ExportFormat::GetExtension(ExportFormat::kFormatCount).isEmpty());
}

TEST(CodecExportFormat, AllFormatsHaveNames)
{
	using olive::ExportFormat;

	for (int i = 0; i < ExportFormat::kFormatCount; ++i) {
		const auto fmt = static_cast<ExportFormat::Format>(i);
		EXPECT_FALSE(ExportFormat::GetName(fmt).isEmpty()) << "Format " << i;
	}
}

TEST(CodecExportFormat, AllFormatsHaveAtLeastOneCodecList)
{
	using olive::ExportFormat;

	for (int i = 0; i < ExportFormat::kFormatCount; ++i) {
		const auto fmt = static_cast<ExportFormat::Format>(i);
		EXPECT_FALSE(ExportFormat::GetVideoCodecs(fmt).isEmpty() &&
					 ExportFormat::GetAudioCodecs(fmt).isEmpty() &&
					 ExportFormat::GetSubtitleCodecs(fmt).isEmpty())
			<< "Format " << i;
	}
}

TEST(CodecExportFormat, CodecLists)
{
	using olive::ExportCodec;
	using olive::ExportFormat;

	const QList<ExportCodec::Codec> matroska_video =
		ExportFormat::GetVideoCodecs(ExportFormat::kFormatMatroska);
	EXPECT_TRUE(matroska_video.contains(ExportCodec::kCodecH264));
	EXPECT_TRUE(matroska_video.contains(ExportCodec::kCodecVP9));

	const QList<ExportCodec::Codec> ogg_audio =
		ExportFormat::GetAudioCodecs(ExportFormat::kFormatOgg);
	EXPECT_TRUE(ogg_audio.contains(ExportCodec::kCodecOpus));
	EXPECT_TRUE(ogg_audio.contains(ExportCodec::kCodecVorbis));

	const QList<ExportCodec::Codec> png_video =
		ExportFormat::GetVideoCodecs(ExportFormat::kFormatPNG);
	EXPECT_EQ(png_video, QList<ExportCodec::Codec>{ ExportCodec::kCodecPNG });

	const QList<ExportCodec::Codec> srt_subs =
		ExportFormat::GetSubtitleCodecs(ExportFormat::kFormatMatroska);
	EXPECT_EQ(srt_subs, QList<ExportCodec::Codec>{ ExportCodec::kCodecSRT });

	const QList<ExportCodec::Codec> wav_audio =
		ExportFormat::GetAudioCodecs(ExportFormat::kFormatWAV);
	EXPECT_EQ(wav_audio, QList<ExportCodec::Codec>{ ExportCodec::kCodecPCM });
}

TEST(CodecExportFormat, MPEG4ContainsH264AndAAC)
{
	using olive::ExportCodec;
	using olive::ExportFormat;

	EXPECT_TRUE(ExportFormat::GetVideoCodecs(ExportFormat::kFormatMPEG4Video)
					.contains(ExportCodec::kCodecH264));
	EXPECT_TRUE(ExportFormat::GetAudioCodecs(ExportFormat::kFormatMPEG4Audio)
					.contains(ExportCodec::kCodecAAC));
}
