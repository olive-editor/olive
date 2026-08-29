#include <gtest/gtest.h>

#include "render/plugin/pluginrenderer.h"

TEST(PluginRendererReadback, BytesToPixels)
{
	olive::VideoParams params(16, 16, olive::core::PixelFormat::U8, 4,
							  olive::core::rational(1, 1),
							  olive::VideoParams::kInterlaceNone, 1);

	const int bytes_per_pixel = olive::VideoParams::GetBytesPerPixel(
		params.format(), params.channel_count());
	ASSERT_EQ(bytes_per_pixel, 4);

	EXPECT_EQ(olive::plugin::detail::BytesToPixels(64, params), 16);
	EXPECT_EQ(olive::plugin::detail::BytesToPixels(0, params), 0);
	EXPECT_EQ(olive::plugin::detail::BytesToPixels(-1, params), 0);
}
