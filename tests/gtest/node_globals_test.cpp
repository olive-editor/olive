#include <gtest/gtest.h>

#include "node/globals.h"
#include "render/videoparams.h"
#include "render/loopmode.h"

TEST(NodeGlobals, DefaultConstruction)
{
	olive::NodeGlobals globals;
	EXPECT_EQ(globals.vparams().width(), 0);
	EXPECT_EQ(globals.vparams().height(), 0);
	EXPECT_EQ(globals.aparams().sample_rate(), 0);
}

TEST(NodeGlobals, ConstructedWithParams)
{
	olive::VideoParams video_params(1920, 1080, olive::PixelFormat::F32, 4);
	olive::AudioParams audio_params;
	audio_params.set_sample_rate(48000);
	audio_params.set_channel_layout(olive::core::kChannelLayoutStereo);

	olive::TimeRange time(olive::core::rational(1, 24),
						  olive::core::rational(2, 24));
	olive::NodeGlobals globals(video_params, audio_params, time,
							   olive::LoopMode::kLoopModeLoop);

	EXPECT_EQ(globals.vparams().width(), 1920);
	EXPECT_EQ(globals.vparams().height(), 1080);
	EXPECT_EQ(globals.aparams().sample_rate(), 48000);
	EXPECT_EQ(globals.loop_mode(), olive::LoopMode::kLoopModeLoop);
	EXPECT_EQ(globals.time().in(), olive::core::rational(1, 24));
	EXPECT_EQ(globals.time().out(), olive::core::rational(2, 24));
}

TEST(NodeGlobals, RationalConstructorExpandsToFrame)
{
	olive::VideoParams video_params(1280, 720, olive::PixelFormat::F32, 4);
	video_params.set_frame_rate(24);
	olive::AudioParams audio_params;
	audio_params.set_sample_rate(44100);

	olive::NodeGlobals globals(video_params, audio_params,
							   olive::core::rational(0, 1),
							   olive::LoopMode::kLoopModeClamp);

	EXPECT_EQ(globals.time().in(), olive::core::rational(0, 1));
	EXPECT_EQ(globals.time().out(), olive::core::rational(1, 24));
}
