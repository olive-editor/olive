#include <gtest/gtest.h>

#include "olive/core/util/color.h"

using namespace olive::core;

TEST(CoreColor, DefaultConstruction)
{
	Color c;
	EXPECT_FLOAT_EQ(c.red(), 0.0f);
	EXPECT_FLOAT_EQ(c.green(), 0.0f);
	EXPECT_FLOAT_EQ(c.blue(), 0.0f);
	EXPECT_FLOAT_EQ(c.alpha(), 0.0f);
}

TEST(CoreColor, ValueConstruction)
{
	Color c(0.1f, 0.2f, 0.3f, 0.4f);
	EXPECT_FLOAT_EQ(c.red(), 0.1f);
	EXPECT_FLOAT_EQ(c.green(), 0.2f);
	EXPECT_FLOAT_EQ(c.blue(), 0.3f);
	EXPECT_FLOAT_EQ(c.alpha(), 0.4f);
}

TEST(CoreColor, SettersAndDataAccess)
{
	Color c;
	c.set_red(0.5f);
	c.set_green(0.6f);
	c.set_blue(0.7f);
	c.set_alpha(0.8f);

	EXPECT_FLOAT_EQ(c.data()[0], 0.5f);
	EXPECT_FLOAT_EQ(c.data()[1], 0.6f);
	EXPECT_FLOAT_EQ(c.data()[2], 0.7f);
	EXPECT_FLOAT_EQ(c.data()[3], 0.8f);
}

TEST(CoreColor, FromHsvRed)
{
	Color c = Color::fromHsv(0.0f, 1.0f, 1.0f);
	EXPECT_NEAR(c.red(), 1.0f, 0.001f);
	EXPECT_NEAR(c.green(), 0.0f, 0.001f);
	EXPECT_NEAR(c.blue(), 0.0f, 0.001f);
}

TEST(CoreColor, FromHsvGreen)
{
	Color c = Color::fromHsv(120.0f, 1.0f, 1.0f);
	EXPECT_NEAR(c.red(), 0.0f, 0.001f);
	EXPECT_NEAR(c.green(), 1.0f, 0.001f);
	EXPECT_NEAR(c.blue(), 0.0f, 0.001f);
}

TEST(CoreColor, FromHsvBlue)
{
	Color c = Color::fromHsv(240.0f, 1.0f, 1.0f);
	EXPECT_NEAR(c.red(), 0.0f, 0.001f);
	EXPECT_NEAR(c.green(), 0.0f, 0.001f);
	EXPECT_NEAR(c.blue(), 1.0f, 0.001f);
}

TEST(CoreColor, HsvRoundTrip)
{
	Color original(0.8f, 0.4f, 0.2f);
	float h, s, v;
	original.toHsv(&h, &s, &v);

	EXPECT_NEAR(original.hsv_hue(), h, 0.001f);
	EXPECT_NEAR(original.hsv_saturation(), s, 0.001f);
	EXPECT_NEAR(original.value(), v, 0.001f);
}

TEST(CoreColor, HslRoundTrip)
{
	Color original(0.2f, 0.5f, 0.8f);
	float h, s, l;
	original.toHsl(&h, &s, &l);

	EXPECT_NEAR(original.hsl_hue(), h, 0.001f);
	EXPECT_NEAR(original.hsl_saturation(), s, 0.001f);
	EXPECT_NEAR(original.lightness(), l, 0.001f);
}

TEST(CoreColor, ArithmeticOperators)
{
	Color a(1.0f, 2.0f, 3.0f, 4.0f);
	Color b(0.5f, 0.5f, 0.5f, 0.5f);

	Color sum = a + b;
	EXPECT_FLOAT_EQ(sum.red(), 1.5f);

	Color diff = a - b;
	EXPECT_FLOAT_EQ(diff.red(), 0.5f);

	Color scaled = a * 2.0f;
	EXPECT_FLOAT_EQ(scaled.red(), 2.0f);

	Color divided = a / 2.0f;
	EXPECT_FLOAT_EQ(divided.red(), 0.5f);

	Color added_scalar = a + 1.0f;
	EXPECT_FLOAT_EQ(added_scalar.red(), 2.0f);
}

TEST(CoreColor, CompoundAssignment)
{
	Color c(1.0f, 2.0f, 3.0f, 4.0f);
	c += Color(0.5f, 0.5f, 0.5f, 0.5f);
	EXPECT_FLOAT_EQ(c.red(), 1.5f);

	c *= 2.0f;
	EXPECT_FLOAT_EQ(c.red(), 3.0f);
}

TEST(CoreColor, GetRoughLuminance)
{
	Color white(1.0f, 1.0f, 1.0f);
	EXPECT_FLOAT_EQ(white.GetRoughLuminance(), 1.0f);

	Color black(0.0f, 0.0f, 0.0f);
	EXPECT_FLOAT_EQ(black.GetRoughLuminance(), 0.0f);
}

TEST(CoreColor, ToDataAndFromDataU8)
{
	Color c(1.0f, 0.5f, 0.0f, 1.0f);
	uint8_t data[4];
	c.toData(reinterpret_cast<char *>(data), PixelFormat::U8, 4);

	EXPECT_EQ(data[0], 255u);
	EXPECT_EQ(data[1], 127u);
	EXPECT_EQ(data[2], 0u);
	EXPECT_EQ(data[3], 255u);

	Color restored = Color::fromData(reinterpret_cast<const char *>(data),
									 PixelFormat::U8, 4);
	EXPECT_NEAR(restored.red(), 1.0f, 0.01f);
	EXPECT_NEAR(restored.green(), 0.5f, 0.01f);
}

TEST(CoreColor, ToDataAndFromDataF32)
{
	Color c(0.25f, 0.5f, 0.75f, 1.0f);
	float data[4];
	c.toData(reinterpret_cast<char *>(data), PixelFormat::F32, 4);

	EXPECT_FLOAT_EQ(data[0], 0.25f);
	EXPECT_FLOAT_EQ(data[1], 0.5f);
	EXPECT_FLOAT_EQ(data[2], 0.75f);
	EXPECT_FLOAT_EQ(data[3], 1.0f);

	Color restored = Color::fromData(reinterpret_cast<const char *>(data),
									 PixelFormat::F32, 4);
	EXPECT_FLOAT_EQ(restored.red(), 0.25f);
}

TEST(CoreColor, ToDataAndFromDataU10)
{
	Color c(1.0f, 0.5f, 0.0f, 1.0f);
	uint32_t data;
	c.toData(reinterpret_cast<char *>(&data), PixelFormat::U10, 4);

	Color restored = Color::fromData(reinterpret_cast<const char *>(&data),
									 PixelFormat::U10, 4);
	EXPECT_NEAR(restored.red(), 1.0f, 0.001f);
	EXPECT_NEAR(restored.green(), 0.5f, 0.001f);
	EXPECT_NEAR(restored.blue(), 0.0f, 0.001f);
}
