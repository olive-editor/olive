#include <gtest/gtest.h>

#include "olive/core/render/sampleformat.h"

TEST(RenderSampleFormat, ByteCountAndStringRoundTrip)
{
	using olive::core::SampleFormat;

	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::INVALID), 0);
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::U8), 1);
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::S16), 2);
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::F32), 4);
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::F64), 8);

	EXPECT_EQ(SampleFormat::to_string(SampleFormat::S16), "s16");
	EXPECT_EQ(SampleFormat::from_string("s16"), SampleFormat::S16);
	EXPECT_EQ(SampleFormat::from_string(""), SampleFormat::INVALID);
	EXPECT_EQ(SampleFormat::from_string("unknown"), SampleFormat::INVALID);
}

TEST(RenderSampleFormat, PackedAndPlanarChecks)
{
	using olive::core::SampleFormat;

	EXPECT_TRUE(SampleFormat::is_packed(SampleFormat::S16));
	EXPECT_FALSE(SampleFormat::is_packed(SampleFormat::S16P));
	EXPECT_TRUE(SampleFormat::is_planar(SampleFormat::S16P));
	EXPECT_FALSE(SampleFormat::is_planar(SampleFormat::S16));
}
