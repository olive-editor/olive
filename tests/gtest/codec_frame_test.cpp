#include <gtest/gtest.h>

#include "codec/frame.h"

TEST(CodecFrame, DefaultState)
{
	olive::Frame frame;
	EXPECT_EQ(frame.width(), 0);
	EXPECT_EQ(frame.height(), 0);
	EXPECT_EQ(frame.format(), olive::core::PixelFormat::INVALID);
	EXPECT_FALSE(frame.is_allocated());
	EXPECT_EQ(frame.data(), nullptr);
}

TEST(CodecFrame, CreateAllocatesForParams)
{
	olive::VideoParams params(64, 32, olive::core::PixelFormat::U8,
							  olive::VideoParams::kRGBAChannelCount);

	olive::FramePtr frame = olive::Frame::Create();
	frame->set_video_params(params);
	frame->allocate();

	EXPECT_TRUE(frame->is_allocated());
	EXPECT_NE(frame->data(), nullptr);
	EXPECT_EQ(frame->width(), 64);
	EXPECT_EQ(frame->height(), 32);
	EXPECT_EQ(frame->format(), olive::core::PixelFormat::U8);
}

TEST(CodecFrame, AllocateMatchesLineSize)
{
	olive::VideoParams params(64, 32, olive::core::PixelFormat::U8,
							  olive::VideoParams::kRGBAChannelCount);

	olive::FramePtr frame = olive::Frame::Create();
	frame->set_video_params(params);
	frame->allocate();

	EXPECT_EQ(frame->linesize_bytes(), 64 * 4);
	EXPECT_EQ(frame->allocated_size(), 64 * 4 * 32);
}

TEST(CodecFrame, DestroyDeallocatesData)
{
	olive::FramePtr frame = olive::Frame::Create();
	frame->set_video_params(
		olive::VideoParams(8, 8, olive::core::PixelFormat::U8,
						   olive::VideoParams::kRGBAChannelCount));
	frame->allocate();
	EXPECT_TRUE(frame->is_allocated());

	frame->destroy();
	EXPECT_FALSE(frame->is_allocated());
	EXPECT_EQ(frame->data(), nullptr);
}

TEST(CodecFrame, AllocateInvalidParamsFails)
{
	olive::Frame frame;
	EXPECT_FALSE(frame.allocate());
}

TEST(CodecFrame, DoubleAllocateReturnsTrue)
{
	olive::VideoParams params(8, 8, olive::core::PixelFormat::U8,
							  olive::VideoParams::kRGBAChannelCount);
	olive::FramePtr frame = olive::Frame::Create();
	frame->set_video_params(params);
	EXPECT_TRUE(frame->allocate());
	EXPECT_TRUE(frame->allocate());
}

TEST(CodecFrame, ContainsPixel)
{
	olive::VideoParams params(8, 8, olive::core::PixelFormat::U8,
							  olive::VideoParams::kRGBAChannelCount);
	olive::FramePtr frame = olive::Frame::Create();
	frame->set_video_params(params);

	EXPECT_FALSE(frame->contains_pixel(0, 0));

	frame->allocate();
	EXPECT_TRUE(frame->contains_pixel(0, 0));
	EXPECT_TRUE(frame->contains_pixel(7, 7));
	EXPECT_FALSE(frame->contains_pixel(8, 0));
	EXPECT_FALSE(frame->contains_pixel(0, 8));
	EXPECT_FALSE(frame->contains_pixel(-1, 0));
}

TEST(CodecFrame, GetPixelOutOfBoundsReturnsBlack)
{
	olive::VideoParams params(8, 8, olive::core::PixelFormat::U8,
							  olive::VideoParams::kRGBAChannelCount);
	olive::FramePtr frame = olive::Frame::Create();
	frame->set_video_params(params);
	frame->allocate();

	olive::core::Color c = frame->get_pixel(-1, 0);
	EXPECT_FLOAT_EQ(c.red(), 0.0f);
}

TEST(CodecFrame, SetAndGetPixel)
{
	olive::VideoParams params(8, 8, olive::core::PixelFormat::U8,
							  olive::VideoParams::kRGBAChannelCount);
	olive::FramePtr frame = olive::Frame::Create();
	frame->set_video_params(params);
	frame->allocate();

	olive::core::Color red(1.0f, 0.0f, 0.0f, 1.0f);
	frame->set_pixel(3, 3, red);

	olive::core::Color got = frame->get_pixel(3, 3);
	EXPECT_NEAR(got.red(), 1.0f, 0.01f);
	EXPECT_NEAR(got.green(), 0.0f, 0.01f);
	EXPECT_NEAR(got.blue(), 0.0f, 0.01f);
}

TEST(CodecFrame, TimestampRoundTrip)
{
	olive::FramePtr frame = olive::Frame::Create();
	frame->set_timestamp(olive::core::rational(5, 1));
	EXPECT_EQ(frame->timestamp(), olive::core::rational(5, 1));
}

TEST(CodecFrame, GenerateLineSizeBytes)
{
	EXPECT_EQ(olive::Frame::generate_linesize_bytes(
				  64, olive::core::PixelFormat::U8,
				  olive::VideoParams::kRGBAChannelCount),
			  64 * 4);
}

TEST(CodecFrame, InterlaceFrames)
{
	olive::VideoParams params(4, 4, olive::core::PixelFormat::U8,
							  olive::VideoParams::kRGBAChannelCount);

	olive::FramePtr top = olive::Frame::Create();
	top->set_video_params(params);
	top->allocate();
	memset(top->data(), 0xFF, top->allocated_size());

	olive::FramePtr bottom = olive::Frame::Create();
	bottom->set_video_params(params);
	bottom->allocate();
	memset(bottom->data(), 0x00, bottom->allocated_size());

	olive::FramePtr interlaced = olive::Frame::Interlace(top, bottom);
	ASSERT_NE(interlaced, nullptr);
	EXPECT_EQ(interlaced->width(), 4);
	EXPECT_EQ(interlaced->height(), 4);
}

TEST(CodecFrame, InterlaceIncompatibleReturnsNull)
{
	olive::FramePtr top = olive::Frame::Create();
	top->set_video_params(
		olive::VideoParams(4, 4, olive::core::PixelFormat::U8,
						   olive::VideoParams::kRGBAChannelCount));
	top->allocate();

	olive::FramePtr bottom = olive::Frame::Create();
	bottom->set_video_params(
		olive::VideoParams(8, 8, olive::core::PixelFormat::U8,
						   olive::VideoParams::kRGBAChannelCount));
	bottom->allocate();

	EXPECT_EQ(olive::Frame::Interlace(top, bottom), nullptr);
}

TEST(CodecFrame, ConvertU8ToU16)
{
	olive::VideoParams params(4, 4, olive::core::PixelFormat::U8,
							  olive::VideoParams::kRGBAChannelCount);
	olive::FramePtr frame = olive::Frame::Create();
	frame->set_video_params(params);
	frame->allocate();

	olive::FramePtr converted = frame->convert(olive::core::PixelFormat::U16);
	ASSERT_NE(converted, nullptr);
	EXPECT_EQ(converted->format(), olive::core::PixelFormat::U16);
	EXPECT_EQ(converted->width(), 4);
	EXPECT_EQ(converted->height(), 4);
}
