#include <gtest/gtest.h>

#include "common/Current.h"
#include "render/videoparams.h"
#include "olive/core/render/audioparams.h"

TEST(CommonCurrent, SetAndGetVideoParams)
{
	olive::VideoParams params;
	params.set_width(1920);
	params.set_height(1080);
	Current::getInstance().setCurrentVideoParams(params);

	const olive::VideoParams &stored =
		Current::getInstance().currentVideoParams();
	EXPECT_EQ(stored.width(), 1920);
	EXPECT_EQ(stored.height(), 1080);
}

TEST(CommonCurrent, SetAndGetAudioParams)
{
	olive::AudioParams params;
	params.set_sample_rate(48000);
	Current::getInstance().setCurrentAudioParams(params);

	const olive::AudioParams &stored =
		Current::getInstance().currentAudioParams();
	EXPECT_EQ(stored.sample_rate(), 48000);
}
