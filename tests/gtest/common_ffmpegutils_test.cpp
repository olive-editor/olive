#include <gtest/gtest.h>

#include "common/ffmpegutils.h"

using namespace olive;

TEST(CommonFFmpegUtils, GetNativeSampleFormatMapsCorrectly)
{
	EXPECT_EQ(FFmpegUtils::GetNativeSampleFormat(FB_SAMPLE_FMT_U8),
			  SampleFormat::U8);
	EXPECT_EQ(FFmpegUtils::GetNativeSampleFormat(FB_SAMPLE_FMT_S16),
			  SampleFormat::S16);
	EXPECT_EQ(FFmpegUtils::GetNativeSampleFormat(FB_SAMPLE_FMT_S32),
			  SampleFormat::S32);
	EXPECT_EQ(FFmpegUtils::GetNativeSampleFormat(FB_SAMPLE_FMT_S64),
			  SampleFormat::S64);
	EXPECT_EQ(FFmpegUtils::GetNativeSampleFormat(FB_SAMPLE_FMT_FLT),
			  SampleFormat::F32);
	EXPECT_EQ(FFmpegUtils::GetNativeSampleFormat(FB_SAMPLE_FMT_DBL),
			  SampleFormat::F64);
	EXPECT_EQ(FFmpegUtils::GetNativeSampleFormat(FB_SAMPLE_FMT_U8P),
			  SampleFormat::U8P);
	EXPECT_EQ(FFmpegUtils::GetNativeSampleFormat(FB_SAMPLE_FMT_S16P),
			  SampleFormat::S16P);
	EXPECT_EQ(FFmpegUtils::GetNativeSampleFormat(FB_SAMPLE_FMT_S32P),
			  SampleFormat::S32P);
	EXPECT_EQ(FFmpegUtils::GetNativeSampleFormat(FB_SAMPLE_FMT_S64P),
			  SampleFormat::S64P);
	EXPECT_EQ(FFmpegUtils::GetNativeSampleFormat(FB_SAMPLE_FMT_FLTP),
			  SampleFormat::F32P);
	EXPECT_EQ(FFmpegUtils::GetNativeSampleFormat(FB_SAMPLE_FMT_DBLP),
			  SampleFormat::F64P);
	EXPECT_EQ(FFmpegUtils::GetNativeSampleFormat(FB_SAMPLE_FMT_NONE),
			  SampleFormat::INVALID);
}

TEST(CommonFFmpegUtils, GetFFmpegSampleFormatMapsCorrectly)
{
	EXPECT_EQ(FFmpegUtils::GetFFmpegSampleFormat(SampleFormat::U8),
			  FB_SAMPLE_FMT_U8);
	EXPECT_EQ(FFmpegUtils::GetFFmpegSampleFormat(SampleFormat::S16),
			  FB_SAMPLE_FMT_S16);
	EXPECT_EQ(FFmpegUtils::GetFFmpegSampleFormat(SampleFormat::S32),
			  FB_SAMPLE_FMT_S32);
	EXPECT_EQ(FFmpegUtils::GetFFmpegSampleFormat(SampleFormat::S64),
			  FB_SAMPLE_FMT_S64);
	EXPECT_EQ(FFmpegUtils::GetFFmpegSampleFormat(SampleFormat::F32),
			  FB_SAMPLE_FMT_FLT);
	EXPECT_EQ(FFmpegUtils::GetFFmpegSampleFormat(SampleFormat::F64),
			  FB_SAMPLE_FMT_DBL);
	EXPECT_EQ(FFmpegUtils::GetFFmpegSampleFormat(SampleFormat::U8P),
			  FB_SAMPLE_FMT_U8P);
	EXPECT_EQ(FFmpegUtils::GetFFmpegSampleFormat(SampleFormat::S16P),
			  FB_SAMPLE_FMT_S16P);
	EXPECT_EQ(FFmpegUtils::GetFFmpegSampleFormat(SampleFormat::S32P),
			  FB_SAMPLE_FMT_S32P);
	EXPECT_EQ(FFmpegUtils::GetFFmpegSampleFormat(SampleFormat::S64P),
			  FB_SAMPLE_FMT_S64P);
	EXPECT_EQ(FFmpegUtils::GetFFmpegSampleFormat(SampleFormat::F32P),
			  FB_SAMPLE_FMT_FLTP);
	EXPECT_EQ(FFmpegUtils::GetFFmpegSampleFormat(SampleFormat::F64P),
			  FB_SAMPLE_FMT_DBLP);
	EXPECT_EQ(FFmpegUtils::GetFFmpegSampleFormat(SampleFormat::INVALID),
			  FB_SAMPLE_FMT_NONE);
}

TEST(CommonFFmpegUtils, ConvertJPEGSpaceToRegularSpace)
{
	EXPECT_EQ(FFmpegUtils::ConvertJPEGSpaceToRegularSpace(FB_PIX_FMT_YUVJ420P),
			  FB_PIX_FMT_YUV420P);
	EXPECT_EQ(FFmpegUtils::ConvertJPEGSpaceToRegularSpace(FB_PIX_FMT_YUVJ422P),
			  FB_PIX_FMT_YUV422P);
	EXPECT_EQ(FFmpegUtils::ConvertJPEGSpaceToRegularSpace(FB_PIX_FMT_YUVJ444P),
			  FB_PIX_FMT_YUV444P);
	EXPECT_EQ(FFmpegUtils::ConvertJPEGSpaceToRegularSpace(FB_PIX_FMT_YUVJ440P),
			  FB_PIX_FMT_YUV440P);
	EXPECT_EQ(FFmpegUtils::ConvertJPEGSpaceToRegularSpace(FB_PIX_FMT_YUVJ411P),
			  FB_PIX_FMT_YUV411P);
	EXPECT_EQ(FFmpegUtils::ConvertJPEGSpaceToRegularSpace(FB_PIX_FMT_YUV420P),
			  FB_PIX_FMT_YUV420P);
}

TEST(CommonFFmpegUtils, GetCompatiblePixelFormatNative)
{
	EXPECT_EQ(FFmpegUtils::GetCompatiblePixelFormat(PixelFormat::U8),
			  PixelFormat::U8);
	EXPECT_EQ(FFmpegUtils::GetCompatiblePixelFormat(PixelFormat::U10),
			  PixelFormat::U8);
	EXPECT_EQ(FFmpegUtils::GetCompatiblePixelFormat(PixelFormat::U16),
			  PixelFormat::U16);
	EXPECT_EQ(FFmpegUtils::GetCompatiblePixelFormat(PixelFormat::F16),
			  PixelFormat::U16);
	EXPECT_EQ(FFmpegUtils::GetCompatiblePixelFormat(PixelFormat::F32),
			  PixelFormat::U16);
	EXPECT_EQ(FFmpegUtils::GetCompatiblePixelFormat(PixelFormat::INVALID),
			  PixelFormat::INVALID);
}

TEST(CommonFFmpegUtils, GetFFmpegPixelFormat)
{
	EXPECT_EQ(FFmpegUtils::GetFFmpegPixelFormat(PixelFormat::U8,
												VideoParams::kRGBChannelCount),
			  FB_PIX_FMT_RGB24);
	EXPECT_EQ(FFmpegUtils::GetFFmpegPixelFormat(PixelFormat::U16,
												VideoParams::kRGBChannelCount),
			  FB_PIX_FMT_RGB48LE);
	EXPECT_EQ(FFmpegUtils::GetFFmpegPixelFormat(PixelFormat::F32,
												VideoParams::kRGBChannelCount),
			  FB_PIX_FMT_RGBF32LE);
	EXPECT_EQ(FFmpegUtils::GetFFmpegPixelFormat(PixelFormat::U8,
												VideoParams::kRGBAChannelCount),
			  FB_PIX_FMT_RGBA);
	EXPECT_EQ(FFmpegUtils::GetFFmpegPixelFormat(PixelFormat::U16,
												VideoParams::kRGBAChannelCount),
			  FB_PIX_FMT_RGBA64LE);
	EXPECT_EQ(FFmpegUtils::GetFFmpegPixelFormat(PixelFormat::F32,
												VideoParams::kRGBAChannelCount),
			  FB_PIX_FMT_RGBAF32LE);
	EXPECT_EQ(FFmpegUtils::GetFFmpegPixelFormat(PixelFormat::INVALID, 0),
			  FB_PIX_FMT_NONE);
}

TEST(CommonFFmpegUtils, GetCompatiblePixelFormatAV)
{
	int fmt = FFmpegUtils::GetCompatibleBridgePixelFormat(FB_PIX_FMT_YUV420P);
	EXPECT_NE(fmt, FB_PIX_FMT_NONE);

	fmt = FFmpegUtils::GetCompatibleBridgePixelFormat(FB_PIX_FMT_YUV420P,
												PixelFormat::U8);
	EXPECT_EQ(fmt, FB_PIX_FMT_RGBA);
}
