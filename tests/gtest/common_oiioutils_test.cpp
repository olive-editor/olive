#include <gtest/gtest.h>

#include <OpenImageIO/imagebuf.h>

#include <cstring>

#include "codec/frame.h"
#include "common/oiioutils.h"

TEST(CommonOIIOUtils, BaseTypeFromPixelFormat)
{
	using olive::core::PixelFormat;

	EXPECT_EQ(olive::OIIOUtils::GetOIIOBaseTypeFromFormat(PixelFormat::U8),
			  OIIO::TypeDesc::UINT8);
	EXPECT_EQ(olive::OIIOUtils::GetOIIOBaseTypeFromFormat(PixelFormat::U16),
			  OIIO::TypeDesc::UINT16);
	EXPECT_EQ(olive::OIIOUtils::GetOIIOBaseTypeFromFormat(PixelFormat::F16),
			  OIIO::TypeDesc::HALF);
	EXPECT_EQ(olive::OIIOUtils::GetOIIOBaseTypeFromFormat(PixelFormat::F32),
			  OIIO::TypeDesc::FLOAT);
	EXPECT_EQ(olive::OIIOUtils::GetOIIOBaseTypeFromFormat(PixelFormat::U10),
			  OIIO::TypeDesc::UNKNOWN);
	EXPECT_EQ(olive::OIIOUtils::GetOIIOBaseTypeFromFormat(PixelFormat::INVALID),
			  OIIO::TypeDesc::UNKNOWN);
}

TEST(CommonOIIOUtils, PixelFormatFromBaseType)
{
	using olive::core::PixelFormat;

	EXPECT_EQ(olive::OIIOUtils::GetFormatFromOIIOBasetype(OIIO::TypeDesc::UINT8),
			  PixelFormat::U8);
	EXPECT_EQ(
			olive::OIIOUtils::GetFormatFromOIIOBasetype(OIIO::TypeDesc::UINT16),
			PixelFormat::U16);
	EXPECT_EQ(olive::OIIOUtils::GetFormatFromOIIOBasetype(OIIO::TypeDesc::HALF),
			  PixelFormat::F16);
	EXPECT_EQ(olive::OIIOUtils::GetFormatFromOIIOBasetype(OIIO::TypeDesc::FLOAT),
			  PixelFormat::F32);
	EXPECT_EQ(
			olive::OIIOUtils::GetFormatFromOIIOBasetype(OIIO::TypeDesc::UNKNOWN),
			PixelFormat::INVALID);
	EXPECT_EQ(olive::OIIOUtils::GetFormatFromOIIOBasetype(OIIO::TypeDesc::DOUBLE),
			  PixelFormat::INVALID);
	EXPECT_EQ(olive::OIIOUtils::GetFormatFromOIIOBasetype(OIIO::TypeDesc::STRING),
			  PixelFormat::INVALID);
}

TEST(CommonOIIOUtils, FormatRoundTripIsSymmetric)
{
	using olive::core::PixelFormat;

	for (PixelFormat fmt :
		 { PixelFormat::U8, PixelFormat::U16, PixelFormat::F16,
		   PixelFormat::F32 }) {
		EXPECT_EQ(olive::OIIOUtils::GetFormatFromOIIOBasetype(
					  olive::OIIOUtils::GetOIIOBaseTypeFromFormat(fmt)),
				  fmt);
	}
}

TEST(CommonOIIOUtils, PixelAspectRatioDefaultsToOne)
{
	OIIO::ImageSpec spec(16, 16, 4, OIIO::TypeDesc::FLOAT);

	const olive::core::rational par =
		olive::OIIOUtils::GetPixelAspectRatioFromOIIO(spec);

	EXPECT_EQ(par, olive::core::rational(1, 1));
}

TEST(CommonOIIOUtils, PixelAspectRatioIsReadFromAttribute)
{
	OIIO::ImageSpec spec(16, 16, 4, OIIO::TypeDesc::FLOAT);
	spec.attribute("PixelAspectRatio", 2.0f);

	const olive::core::rational par =
		olive::OIIOUtils::GetPixelAspectRatioFromOIIO(spec);

	EXPECT_EQ(par, olive::core::rational(2, 1));
}

TEST(CommonOIIOUtils, FrameBufferRoundTripPreservesPixels)
{
	using olive::core::PixelFormat;

	const olive::VideoParams params(8, 8, PixelFormat::F32,
									olive::VideoParams::kRGBAChannelCount);

	auto frame = olive::Frame::Create();
	frame->set_video_params(params);
	frame->allocate();

	frame->set_pixel(0, 0, olive::Color(0.1f, 0.2f, 0.3f, 1.0f));
	frame->set_pixel(7, 7, olive::Color(0.9f, 0.8f, 0.7f, 0.6f));

	OIIO::ImageSpec spec(params.effective_width(), params.effective_height(),
						 params.channel_count(),
						 olive::OIIOUtils::GetOIIOBaseTypeFromFormat(
							 params.format()));
	OIIO::ImageBuf buf(spec);
	ASSERT_TRUE(buf.initialized());

	olive::OIIOUtils::FrameToBuffer(frame.get(), &buf);

	auto out = olive::Frame::Create();
	out->set_video_params(params);
	out->allocate();
	// Poison the output so a failed transfer is visible.
	std::memset(out->data(), 0xFF, out->allocated_size());

	olive::OIIOUtils::BufferToFrame(&buf, out.get());

	const olive::Color a = out->get_pixel(0, 0);
	EXPECT_NEAR(a.red(), 0.1f, 1e-5f);
	EXPECT_NEAR(a.green(), 0.2f, 1e-5f);
	EXPECT_NEAR(a.blue(), 0.3f, 1e-5f);
	EXPECT_NEAR(a.alpha(), 1.0f, 1e-5f);

	const olive::Color b = out->get_pixel(7, 7);
	EXPECT_NEAR(b.red(), 0.9f, 1e-5f);
	EXPECT_NEAR(b.green(), 0.8f, 1e-5f);
	EXPECT_NEAR(b.blue(), 0.7f, 1e-5f);
	EXPECT_NEAR(b.alpha(), 0.6f, 1e-5f);
}
