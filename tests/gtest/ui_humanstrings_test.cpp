#include <gtest/gtest.h>

#include "ui/humanstrings.h"

TEST(UIHumanStrings, SampleRateToStringAppendsHz)
{
	EXPECT_EQ(olive::HumanStrings::SampleRateToString(44100),
			  QStringLiteral("44100 Hz"));
	EXPECT_EQ(olive::HumanStrings::SampleRateToString(48000),
			  QStringLiteral("48000 Hz"));
	EXPECT_EQ(olive::HumanStrings::SampleRateToString(0), QStringLiteral("0 Hz"));
}

TEST(UIHumanStrings, KnownChannelLayoutsHaveNames)
{
	EXPECT_EQ(olive::HumanStrings::ChannelLayoutToString(
				  olive::kChannelLayoutMono),
			  QStringLiteral("Mono"));
	EXPECT_EQ(olive::HumanStrings::ChannelLayoutToString(
				  olive::kChannelLayoutStereo),
			  QStringLiteral("Stereo"));
	EXPECT_EQ(olive::HumanStrings::ChannelLayoutToString(
				  olive::kChannelLayout2_1),
			  QStringLiteral("2.1"));
	EXPECT_EQ(olive::HumanStrings::ChannelLayoutToString(
				  olive::kChannelLayout5Point1),
			  QStringLiteral("5.1"));
	EXPECT_EQ(olive::HumanStrings::ChannelLayoutToString(
				  olive::kChannelLayout7Point1),
			  QStringLiteral("7.1"));
}

TEST(UIHumanStrings, UnknownChannelLayoutFallsBackToHex)
{
	const QString s = olive::HumanStrings::ChannelLayoutToString(0x1234);

	EXPECT_TRUE(s.startsWith(QStringLiteral("Unknown (0x")));
	EXPECT_TRUE(s.contains(QStringLiteral("1234")));
}

TEST(UIHumanStrings, AllSampleFormatsHaveNonEmptyNames)
{
	using olive::core::SampleFormat;

	for (SampleFormat fmt :
		 { SampleFormat::U8, SampleFormat::S16, SampleFormat::S32,
		   SampleFormat::S64, SampleFormat::F32, SampleFormat::F64,
		   SampleFormat::U8P, SampleFormat::S16P, SampleFormat::S32P,
		   SampleFormat::S64P, SampleFormat::F32P, SampleFormat::F64P }) {
		const QString s = olive::HumanStrings::FormatToString(fmt);
		EXPECT_FALSE(s.isEmpty());
		EXPECT_FALSE(s.startsWith(QStringLiteral("Unknown")));
	}
}

TEST(UIHumanStrings, PackedAndPlanarFormatsAreDistinguished)
{
	using olive::core::SampleFormat;

	EXPECT_TRUE(olive::HumanStrings::FormatToString(SampleFormat::F32)
					.contains(QStringLiteral("Packed")));
	EXPECT_TRUE(olive::HumanStrings::FormatToString(SampleFormat::F32P)
					.contains(QStringLiteral("Planar")));
}

TEST(UIHumanStrings, InvalidSampleFormatFallsBackToHex)
{
	const QString s = olive::HumanStrings::FormatToString(
		olive::core::SampleFormat::INVALID);

	EXPECT_TRUE(s.startsWith(QStringLiteral("Unknown (0x")));
}
