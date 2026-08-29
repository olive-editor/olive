#include <gtest/gtest.h>

#include "ofxImageEffect.h"
#include "ofxhClip.h"
#include "pluginSupport/OliveClip.h"

namespace
{
olive::VideoParams MakeParams(int width, int height,
							  olive::core::PixelFormat format, int channels,
							  bool premultiplied)
{
	olive::VideoParams params;
	params.set_width(width);
	params.set_height(height);
	params.set_format(format);
	params.set_channel_count(channels);
	params.set_premultiplied_alpha(premultiplied);
	return params;
}
}

TEST(PluginSupportClip, PropertyGetters)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	olive::VideoParams params =
		MakeParams(1920, 1080, olive::core::PixelFormat::U16, 3, false);
	params.set_pixel_aspect_ratio(olive::core::rational(2, 1));
	params.set_frame_rate(olive::core::rational(30, 1));
	params.set_start_time(2);
	params.set_duration(4);
	params.set_interlacing(olive::VideoParams::kInterlacedTopFirst);

	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	EXPECT_EQ(clip.getUnmappedBitDepth(), kOfxBitDepthShort);
	EXPECT_EQ(clip.getUnmappedComponents(), kOfxImageComponentRGB);
	EXPECT_EQ(clip.getPremult(), kOfxImageUnPreMultiplied);
	EXPECT_DOUBLE_EQ(clip.getAspectRatio(), 2.0);
	EXPECT_DOUBLE_EQ(clip.getFrameRate(), 30.0);

	double start_frame = 0.0;
	double end_frame = 0.0;
	clip.getFrameRange(start_frame, end_frame);
	EXPECT_DOUBLE_EQ(start_frame, 60.0);
	EXPECT_DOUBLE_EQ(end_frame, 180.0);

	EXPECT_EQ(clip.getFieldOrder(), kOfxImageFieldUpper);
	EXPECT_DOUBLE_EQ(clip.getUnmappedFrameRate(), 30.0);

	clip.getUnmappedFrameRange(start_frame, end_frame);
	EXPECT_DOUBLE_EQ(start_frame, 60.0);
	EXPECT_DOUBLE_EQ(end_frame, 180.0);

	EXPECT_FALSE(clip.getContinuousSamples());
	EXPECT_FALSE(clip.getConnected());
}

TEST(PluginSupportClip, GetImageClampsBoundsAndCachesOutput)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	olive::VideoParams params =
		MakeParams(100, 80, olive::core::PixelFormat::U8, 4, true);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	OfxRectD optional_bounds = { -10.0, -10.0, 200.0, 200.0 };
	OFX::Host::ImageEffect::Image *image = clip.getImage(0.0, &optional_bounds);
	ASSERT_NE(image, nullptr);

	auto *olive_image = static_cast<olive::plugin::Image *>(image);
	EXPECT_EQ(olive_image->width(), 100);
	EXPECT_EQ(olive_image->height(), 80);

	OFX::Host::ImageEffect::Image *image_again = clip.getImage(0.0, nullptr);
	EXPECT_EQ(image, image_again);
}

TEST(PluginSupportClip, GetImageCachesImageForNonOutput)
{
	OFX::Host::ImageEffect::ClipDescriptor desc("Source");
	olive::VideoParams params =
		MakeParams(64, 64, olive::core::PixelFormat::U8, 4, false);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	EXPECT_FALSE(clip.getConnected());

	OFX::Host::ImageEffect::Image *first = clip.getImage(0.0, nullptr);
	OFX::Host::ImageEffect::Image *second = clip.getImage(0.0, nullptr);

	EXPECT_NE(first, nullptr);
	EXPECT_NE(second, nullptr);
	// On-demand images are cached per time, so both fetches return the
	// same image and the clip now reports as connected.
	EXPECT_EQ(first, second);
	EXPECT_TRUE(clip.getConnected());

	first->releaseReference();
	second->releaseReference();
}
