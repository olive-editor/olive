#include <gtest/gtest.h>

#include "tool/tool.h"
#include "ui/humanstrings.h"

TEST(ModuleSmoke, ToolAddableObjectNames)
{
	EXPECT_FALSE(
		olive::Tool::GetAddableObjectName(olive::Tool::kAddableEmpty).isEmpty());
	EXPECT_FALSE(
		olive::Tool::GetAddableObjectName(olive::Tool::kAddableBars).isEmpty());
	EXPECT_FALSE(
		olive::Tool::GetAddableObjectName(olive::Tool::kAddableShape).isEmpty());
	EXPECT_FALSE(
		olive::Tool::GetAddableObjectName(olive::Tool::kAddableSolid).isEmpty());
	EXPECT_FALSE(
		olive::Tool::GetAddableObjectName(olive::Tool::kAddableTitle).isEmpty());
	EXPECT_FALSE(
		olive::Tool::GetAddableObjectName(olive::Tool::kAddableTone).isEmpty());
	EXPECT_FALSE(
		olive::Tool::GetAddableObjectName(olive::Tool::kAddableSubtitle)
			.isEmpty());
}

TEST(ModuleSmoke, ToolAddableObjectIds)
{
	EXPECT_EQ(olive::Tool::GetAddableObjectID(olive::Tool::kAddableEmpty),
			  QStringLiteral("empty"));
	EXPECT_EQ(olive::Tool::GetAddableObjectID(olive::Tool::kAddableBars),
			  QStringLiteral("bars"));
	EXPECT_EQ(olive::Tool::GetAddableObjectID(olive::Tool::kAddableShape),
			  QStringLiteral("shape"));
	EXPECT_EQ(olive::Tool::GetAddableObjectID(olive::Tool::kAddableSolid),
			  QStringLiteral("solid"));
	EXPECT_EQ(olive::Tool::GetAddableObjectID(olive::Tool::kAddableTitle),
			  QStringLiteral("title"));
	EXPECT_EQ(olive::Tool::GetAddableObjectID(olive::Tool::kAddableTone),
			  QStringLiteral("tone"));
	EXPECT_EQ(olive::Tool::GetAddableObjectID(olive::Tool::kAddableSubtitle),
			  QStringLiteral("subtitle"));
}

TEST(ModuleSmoke, HumanStringsSampleRate)
{
	EXPECT_FALSE(olive::HumanStrings::SampleRateToString(48000).isEmpty());
	EXPECT_FALSE(olive::HumanStrings::SampleRateToString(44100).isEmpty());
}

TEST(ModuleSmoke, HumanStringsChannelLayout)
{
	EXPECT_FALSE(
		olive::HumanStrings::ChannelLayoutToString(olive::core::kChannelLayoutMono).isEmpty());
	EXPECT_FALSE(olive::HumanStrings::ChannelLayoutToString(olive::core::kChannelLayoutStereo)
					 .isEmpty());
}

TEST(ModuleSmoke, HumanStringsFormat)
{
	EXPECT_FALSE(
		olive::HumanStrings::FormatToString(olive::SampleFormat::U8).isEmpty());
	EXPECT_FALSE(
		olive::HumanStrings::FormatToString(olive::SampleFormat::F32).isEmpty());
}
