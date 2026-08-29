#include <gtest/gtest.h>

#include "olive/core/render/pixelformat.h"

TEST(RenderPixelFormat, ByteCountAndString)
{
	using olive::core::PixelFormat;

	EXPECT_EQ(PixelFormat::byte_count(PixelFormat::INVALID), 0);
	EXPECT_EQ(PixelFormat::byte_count(PixelFormat::U8), 1);
	EXPECT_EQ(PixelFormat::byte_count(PixelFormat::U16), 2);
	EXPECT_EQ(PixelFormat::byte_count(PixelFormat::F16), 2);
	EXPECT_EQ(PixelFormat::byte_count(PixelFormat::F32), 4);

	EXPECT_EQ(PixelFormat(PixelFormat::U8).to_string(), std::string("u8"));
	EXPECT_EQ(PixelFormat(PixelFormat::INVALID).to_string(), std::string(""));
}

TEST(RenderPixelFormat, FloatChecks)
{
	using olive::core::PixelFormat;

	EXPECT_FALSE(PixelFormat::is_float(PixelFormat::U8));
	EXPECT_FALSE(PixelFormat::is_float(PixelFormat::U16));
	EXPECT_TRUE(PixelFormat::is_float(PixelFormat::F16));
	EXPECT_TRUE(PixelFormat::is_float(PixelFormat::F32));
	EXPECT_FALSE(PixelFormat::is_float(PixelFormat::INVALID));
}
