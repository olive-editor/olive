#include <gtest/gtest.h>

#include <cmath>

#include "audio/audiolevelmeter.h"
#include "olive/core/render/audioparams.h"
#include "olive/core/render/samplebuffer.h"
#include "olive/core/render/sampleformat.h"

namespace
{

olive::core::AudioParams MakeStereoParams()
{
	return olive::core::AudioParams(48000, olive::core::kChannelLayoutStereo,
									olive::core::SampleFormat::F32P);
}

}

TEST(AudioLevelMeter, Silence)
{
	olive::core::SampleBuffer samples(MakeStereoParams(), size_t(480));
	samples.silence();

	const olive::AudioLevelMeter::Stats stats =
		olive::AudioLevelMeter::AnalyzeSampleBuffer(samples);

	ASSERT_EQ(stats.channels.size(), 2);
	EXPECT_TRUE(stats.silence);
	EXPECT_DOUBLE_EQ(stats.max_peak_linear, 0.0);
	EXPECT_DOUBLE_EQ(stats.channels.at(0).peak_linear, 0.0);
	EXPECT_DOUBLE_EQ(stats.channels.at(0).rms_linear, 0.0);
	EXPECT_DOUBLE_EQ(stats.integrated_lufs, -200.0);
}

TEST(AudioLevelMeter, ConstantSignal)
{
	olive::core::SampleBuffer samples(MakeStereoParams(), size_t(480));
	for (int channel = 0; channel < samples.channel_count(); channel++) {
		float *data = samples.data(channel);
		for (size_t i = 0; i < samples.sample_count(); i++) {
			data[i] = 0.5f;
		}
	}

	const olive::AudioLevelMeter::Stats stats =
		olive::AudioLevelMeter::AnalyzeSampleBuffer(samples);

	ASSERT_EQ(stats.channels.size(), 2);
	EXPECT_FALSE(stats.silence);
	EXPECT_DOUBLE_EQ(stats.max_peak_linear, 0.5);
	EXPECT_NEAR(stats.channels.at(0).peak_db, -6.0206, 0.0001);
	EXPECT_NEAR(stats.channels.at(0).rms_linear, 0.5, 0.0001);
	EXPECT_NEAR(stats.channels.at(0).vu_db, -6.0206, 0.0001);
	EXPECT_NEAR(stats.integrated_lufs, -6.7116, 0.0001);
}

TEST(AudioLevelMeter, PerChannelPeaksAndRms)
{
	olive::core::SampleBuffer samples(MakeStereoParams(), size_t(4));
	float left[] = { 0.0f, 0.25f, -0.5f, 1.0f };
	float right[] = { 0.0f, -0.25f, 0.25f, -0.25f };
	samples.set(0, left, 4);
	samples.set(1, right, 4);

	const olive::AudioLevelMeter::Stats stats =
		olive::AudioLevelMeter::AnalyzeSampleBuffer(samples);

	ASSERT_EQ(stats.channels.size(), 2);
	EXPECT_DOUBLE_EQ(stats.channels.at(0).peak_linear, 1.0);
	EXPECT_DOUBLE_EQ(stats.channels.at(1).peak_linear, 0.25);
	EXPECT_NEAR(stats.channels.at(0).rms_linear,
				std::sqrt((0.25 * 0.25 + 0.5 * 0.5 + 1.0) / 4.0), 0.0001);
	EXPECT_NEAR(stats.channels.at(1).rms_linear,
				std::sqrt((0.25 * 0.25 * 3.0) / 4.0), 0.0001);
}
