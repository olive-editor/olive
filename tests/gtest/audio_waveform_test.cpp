/***
  Oak Video Editor - Extended tests for AudioVisualWaveform and AudioProcessor
  Copyright (C) 2026 Oak Team
***/

#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <vector>

#include <QColor>
#include <QImage>
#include <QPainter>

#include "audio/audioprocessor.h"
#include "audio/audiovisualwaveform.h"
#include "olive/core/render/audioparams.h"
#include "olive/core/render/samplebuffer.h"
#include "olive/core/render/sampleformat.h"

namespace
{

constexpr int kSampleRate = 48000;

olive::core::AudioParams MakeParams(uint64_t channel_layout)
{
	return olive::core::AudioParams(kSampleRate, channel_layout,
									olive::core::SampleFormat::F32P);
}

// Returns a buffer of `sample_count` samples per channel, every sample set to
// `value`. Constant buffers keep mipmap chunk boundaries irrelevant, so
// summaries can be checked with exact float comparisons.
olive::core::SampleBuffer MakeConstantBuffer(
	const olive::core::AudioParams &params, size_t sample_count, float value)
{
	olive::core::SampleBuffer buffer(params, sample_count);
	for (int ch = 0; ch < buffer.channel_count(); ch++) {
		float *data = buffer.data(ch);
		for (size_t i = 0; i < buffer.sample_count(); i++) {
			data[i] = value;
		}
	}
	return buffer;
}

// Returns one second of mono `first_value` followed by one second of mono
// `second_value`. The split lands exactly on a chunk boundary at every mipmap
// rate when kSampleRate is 48000, so per-range summaries stay exact.
olive::core::SampleBuffer MakeSplitMonoBuffer(float first_value,
											  float second_value)
{
	olive::core::SampleBuffer buffer(
		MakeParams(olive::core::kChannelLayoutMono), size_t(kSampleRate * 2));
	float *data = buffer.data(0);
	for (size_t i = 0; i < size_t(kSampleRate); i++) {
		data[i] = first_value;
	}
	for (size_t i = size_t(kSampleRate); i < buffer.sample_count(); i++) {
		data[i] = second_value;
	}
	return buffer;
}

olive::core::SampleBuffer MakeMonoConstant(size_t sample_count, float value)
{
	return MakeConstantBuffer(MakeParams(olive::core::kChannelLayoutMono),
							  sample_count, value);
}

void ExpectSummary(const olive::AudioVisualWaveform::Sample &summary,
				   size_t channel, float expected_min, float expected_max)
{
	ASSERT_LT(channel, summary.size());
	// ReSumSamples aggregates starting from a zero-initialized accumulator, so
	// a summary always spans zero for single-signed data.
	EXPECT_FLOAT_EQ(summary.at(channel).min, std::min(expected_min, 0.0f));
	EXPECT_FLOAT_EQ(summary.at(channel).max, std::max(expected_max, 0.0f));
}

// Pushes input through the processor, then flushes and drains everything the
// filter graph still holds, returning the accumulated per-plane output.
// Draining after a flush ends at EOF, which AudioProcessor reports as a
// negative return value, so the final Convert result is intentionally unused.
olive::AudioProcessor::Buffer ConvertAndDrain(olive::AudioProcessor &processor,
											  float **input, int nb_samples)
{
	olive::AudioProcessor::Buffer output;
	EXPECT_GE(processor.Convert(input, nb_samples, &output), 0);

	processor.Flush();

	olive::AudioProcessor::Buffer rest;
	processor.Convert(nullptr, 0, &rest);

	if (output.size() < rest.size()) {
		output.resize(rest.size());
	}
	for (int i = 0; i < rest.size(); i++) {
		output[i].append(rest.at(i));
	}
	return output;
}

} // namespace

// ---------------------------------------------------------------------------
// AudioVisualWaveform::OverwriteSamples
// ---------------------------------------------------------------------------

TEST(AudioVisualWaveform, OverwriteSamplesWithZeroChannelsIsIgnored)
{
	olive::AudioVisualWaveform waveform; // channel count defaults to zero

	olive::core::SampleBuffer buffer(
		MakeParams(olive::core::kChannelLayoutStereo), size_t(100));
	waveform.OverwriteSamples(buffer, kSampleRate, olive::core::rational(0));

	// Nothing is written and no length is recorded
	EXPECT_EQ(waveform.length(), olive::core::rational(0));
	EXPECT_TRUE(waveform
					.GetSummaryFromTime(olive::core::rational(0),
										olive::core::rational(1))
					.empty());
}

TEST(AudioVisualWaveform, OverwriteSamplesAtNonZeroStartSetsLength)
{
	olive::AudioVisualWaveform waveform;
	waveform.set_channel_count(1);

	waveform.OverwriteSamples(MakeMonoConstant(size_t(kSampleRate), 0.75f),
							  kSampleRate, olive::core::rational(2));

	// length() tracks the absolute end time of the written data
	EXPECT_EQ(waveform.length(), olive::core::rational(3));

	const olive::AudioVisualWaveform::Sample summary =
		waveform.GetSummaryFromTime(olive::core::rational(2),
									olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.75f, 0.75f);
}

TEST(AudioVisualWaveform, OverwriteSamplesReplacesPreviousData)
{
	olive::AudioVisualWaveform waveform;
	waveform.set_channel_count(1);

	waveform.OverwriteSamples(MakeMonoConstant(size_t(kSampleRate), 0.8f),
							  kSampleRate, olive::core::rational(0));
	waveform.OverwriteSamples(MakeMonoConstant(size_t(kSampleRate), 0.2f),
							  kSampleRate, olive::core::rational(0));

	// Overwriting must replace, not mix or extend
	EXPECT_EQ(waveform.length(), olive::core::rational(1));

	const olive::AudioVisualWaveform::Sample summary =
		waveform.GetSummaryFromTime(olive::core::rational(0),
									olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.2f, 0.2f);
}

TEST(AudioVisualWaveform, OverwriteSamplesAfterGapLeavesSilence)
{
	olive::AudioVisualWaveform waveform;
	waveform.set_channel_count(1);

	waveform.OverwriteSamples(MakeMonoConstant(size_t(kSampleRate), 0.25f),
							  kSampleRate, olive::core::rational(0));
	waveform.OverwriteSamples(MakeMonoConstant(size_t(kSampleRate), 0.75f),
							  kSampleRate, olive::core::rational(2));

	EXPECT_EQ(waveform.length(), olive::core::rational(3));

	olive::AudioVisualWaveform::Sample summary =
		waveform.GetSummaryFromTime(olive::core::rational(0),
									olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.25f, 0.25f);

	// The unwritten gap between the two writes reads back as zeros
	summary = waveform.GetSummaryFromTime(olive::core::rational(1),
										  olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.0f, 0.0f);

	summary = waveform.GetSummaryFromTime(olive::core::rational(2),
										  olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.75f, 0.75f);
}

TEST(AudioVisualWaveform, OverwriteSamplesBeforeExistingDataPrependsZeros)
{
	olive::AudioVisualWaveform waveform;
	waveform.set_channel_count(1);

	waveform.OverwriteSamples(MakeMonoConstant(size_t(kSampleRate), 0.75f),
							  kSampleRate, olive::core::rational(2));
	ASSERT_EQ(waveform.length(), olive::core::rational(3));

	// Writing before the current virtual start pushes the existing data back
	waveform.OverwriteSamples(MakeMonoConstant(size_t(kSampleRate), 0.25f),
							  kSampleRate, olive::core::rational(0));

	olive::AudioVisualWaveform::Sample summary =
		waveform.GetSummaryFromTime(olive::core::rational(0),
									olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.25f, 0.25f);

	// Two seconds (one written, one gap) were prepended
	summary = waveform.GetSummaryFromTime(olive::core::rational(1),
										  olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.0f, 0.0f);

	// The original data is still intact at its absolute position
	summary = waveform.GetSummaryFromTime(olive::core::rational(2),
										  olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.75f, 0.75f);

	// The data now spans [0, 3): prepending via a negative TrimIn keeps the
	// absolute end time tracked by length()
	EXPECT_EQ(waveform.length(), olive::core::rational(3));
}

// ---------------------------------------------------------------------------
// AudioVisualWaveform::GetSummaryFromTime
// ---------------------------------------------------------------------------

TEST(AudioVisualWaveform, GetSummaryFromTimeReturnsExactMinMaxPerChannel)
{
	olive::AudioVisualWaveform waveform;
	waveform.set_channel_count(2);

	olive::core::SampleBuffer buffer(
		MakeParams(olive::core::kChannelLayoutStereo), size_t(kSampleRate));
	float *left = buffer.data(0);
	float *right = buffer.data(1);
	for (size_t i = 0; i < buffer.sample_count(); i++) {
		left[i] = 0.5f;
		right[i] = -0.25f;
	}
	waveform.OverwriteSamples(buffer, kSampleRate, olive::core::rational(0));

	const olive::AudioVisualWaveform::Sample summary =
		waveform.GetSummaryFromTime(olive::core::rational(0),
									olive::core::rational(1));

	ASSERT_EQ(summary.size(), 2);
	ExpectSummary(summary, 0, 0.5f, 0.5f);
	ExpectSummary(summary, 1, -0.25f, -0.25f);
}

TEST(AudioVisualWaveform, GetSummaryFromTimeResolvesDistinctRanges)
{
	olive::AudioVisualWaveform waveform;
	waveform.set_channel_count(1);

	// One second of 0.25 followed by one second of 0.75
	waveform.OverwriteSamples(MakeSplitMonoBuffer(0.25f, 0.75f), kSampleRate,
							  olive::core::rational(0));
	ASSERT_EQ(waveform.length(), olive::core::rational(2));

	olive::AudioVisualWaveform::Sample summary =
		waveform.GetSummaryFromTime(olive::core::rational(0),
									olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.25f, 0.25f);

	summary = waveform.GetSummaryFromTime(olive::core::rational(1),
										  olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.75f, 0.75f);

	// A range covering both seconds merges their extremes
	summary = waveform.GetSummaryFromTime(olive::core::rational(0),
										  olive::core::rational(2));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.25f, 0.75f);
}

TEST(AudioVisualWaveform, GetSummaryFromTimeHandlesSurroundChannels)
{
	olive::AudioVisualWaveform waveform;
	waveform.set_channel_count(6);

	olive::core::SampleBuffer buffer(
		MakeParams(olive::core::kChannelLayout5Point1), size_t(kSampleRate));
	for (int ch = 0; ch < buffer.channel_count(); ch++) {
		float *data = buffer.data(ch);
		for (size_t i = 0; i < buffer.sample_count(); i++) {
			data[i] = 0.1f * float(ch + 1);
		}
	}
	waveform.OverwriteSamples(buffer, kSampleRate, olive::core::rational(0));

	const olive::AudioVisualWaveform::Sample summary =
		waveform.GetSummaryFromTime(olive::core::rational(0),
									olive::core::rational(1));

	ASSERT_EQ(summary.size(), 6);
	for (size_t ch = 0; ch < 6; ch++) {
		const float expected = 0.1f * float(ch + 1);
		ExpectSummary(summary, ch, expected, expected);
	}
}

TEST(AudioVisualWaveform, GetSummaryFromTimeOnEmptyWaveformReturnsNullSamples)
{
	olive::AudioVisualWaveform waveform;
	waveform.set_channel_count(2);

	const olive::AudioVisualWaveform::Sample summary =
		waveform.GetSummaryFromTime(olive::core::rational(0),
									olive::core::rational(1));

	// No data written: one zeroed entry per channel
	ASSERT_EQ(summary.size(), 2);
	ExpectSummary(summary, 0, 0.0f, 0.0f);
	ExpectSummary(summary, 1, 0.0f, 0.0f);
}

TEST(AudioVisualWaveform,
	 GetSummaryFromTimeShorterThanOneSampleReturnsNullSamples)
{
	olive::AudioVisualWaveform waveform;
	waveform.set_channel_count(1);
	waveform.OverwriteSamples(MakeMonoConstant(size_t(kSampleRate), 0.5f),
							  kSampleRate, olive::core::rational(0));

	// Shorter than a single frame even at the highest mipmap rate (1024 Hz),
	// so the request quantizes down to zero frames
	const olive::AudioVisualWaveform::Sample summary =
		waveform.GetSummaryFromTime(olive::core::rational(0),
									olive::core::rational(1, 100000));

	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.0f, 0.0f);
}

// ---------------------------------------------------------------------------
// AudioVisualWaveform::OverwriteSums
// ---------------------------------------------------------------------------

TEST(AudioVisualWaveform, OverwriteSumsCopiesEntireWaveform)
{
	olive::AudioVisualWaveform source;
	source.set_channel_count(1);
	source.OverwriteSamples(MakeSplitMonoBuffer(0.25f, 0.75f), kSampleRate,
							olive::core::rational(0));

	olive::AudioVisualWaveform dest;
	dest.set_channel_count(1);
	dest.OverwriteSums(source, olive::core::rational(0));

	EXPECT_EQ(dest.length(), olive::core::rational(2));

	olive::AudioVisualWaveform::Sample summary =
		dest.GetSummaryFromTime(olive::core::rational(0),
								olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.25f, 0.25f);

	summary = dest.GetSummaryFromTime(olive::core::rational(1),
									  olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.75f, 0.75f);
}

TEST(AudioVisualWaveform, OverwriteSumsAtDestinationOffset)
{
	olive::AudioVisualWaveform source;
	source.set_channel_count(1);
	source.OverwriteSamples(MakeSplitMonoBuffer(0.25f, 0.75f), kSampleRate,
							olive::core::rational(0));

	olive::AudioVisualWaveform dest;
	dest.set_channel_count(1);
	dest.OverwriteSums(source, olive::core::rational(1));

	EXPECT_EQ(dest.length(), olive::core::rational(3));

	// The copied data lands one second later; because the destination's
	// virtual start moved to the destination offset, only [1, 3) can be
	// queried safely
	olive::AudioVisualWaveform::Sample summary =
		dest.GetSummaryFromTime(olive::core::rational(1),
								olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.25f, 0.25f);

	summary = dest.GetSummaryFromTime(olive::core::rational(2),
									  olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.75f, 0.75f);
}

TEST(AudioVisualWaveform, OverwriteSumsWithSourceOffset)
{
	olive::AudioVisualWaveform source;
	source.set_channel_count(1);
	source.OverwriteSamples(MakeSplitMonoBuffer(0.25f, 0.75f), kSampleRate,
							olive::core::rational(0));

	olive::AudioVisualWaveform dest;
	dest.set_channel_count(1);
	dest.OverwriteSums(source, olive::core::rational(0),
					   olive::core::rational(1));

	// Only the source's second second is copied
	EXPECT_EQ(dest.length(), olive::core::rational(1));

	const olive::AudioVisualWaveform::Sample summary =
		dest.GetSummaryFromTime(olive::core::rational(0),
								olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.75f, 0.75f);
}

TEST(AudioVisualWaveform, OverwriteSumsWithLengthLimit)
{
	olive::AudioVisualWaveform source;
	source.set_channel_count(1);
	source.OverwriteSamples(MakeSplitMonoBuffer(0.25f, 0.75f), kSampleRate,
							olive::core::rational(0));

	olive::AudioVisualWaveform dest;
	dest.set_channel_count(1);
	dest.OverwriteSums(source, olive::core::rational(0),
					   olive::core::rational(0), olive::core::rational(1));

	// Only the source's first second is copied
	EXPECT_EQ(dest.length(), olive::core::rational(1));

	const olive::AudioVisualWaveform::Sample summary =
		dest.GetSummaryFromTime(olive::core::rational(0),
								olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.25f, 0.25f);
}

TEST(AudioVisualWaveform, OverwriteSumsWithOffsetBeyondSourceIsIgnored)
{
	olive::AudioVisualWaveform source;
	source.set_channel_count(1);
	source.OverwriteSamples(MakeSplitMonoBuffer(0.25f, 0.75f), kSampleRate,
							olive::core::rational(0));

	olive::AudioVisualWaveform dest;
	dest.set_channel_count(1);
	dest.OverwriteSums(source, olive::core::rational(0),
					   olive::core::rational(10));

	// The offset starts past the end of every source mipmap, so nothing is
	// copied and the destination stays empty
	EXPECT_EQ(dest.length(), olive::core::rational(0));

	const olive::AudioVisualWaveform::Sample summary =
		dest.GetSummaryFromTime(olive::core::rational(0),
								olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.0f, 0.0f);
}

// ---------------------------------------------------------------------------
// AudioVisualWaveform::OverwriteSilence
// ---------------------------------------------------------------------------

TEST(AudioVisualWaveform, OverwriteSilenceZeroesRange)
{
	olive::AudioVisualWaveform waveform;
	waveform.set_channel_count(2);

	waveform.OverwriteSamples(
		MakeConstantBuffer(MakeParams(olive::core::kChannelLayoutStereo),
						   size_t(kSampleRate * 2), 0.8f),
		kSampleRate, olive::core::rational(0));

	// Silence [0.5, 1.5)
	waveform.OverwriteSilence(olive::core::rational(1, 2),
							  olive::core::rational(1));

	olive::AudioVisualWaveform::Sample summary =
		waveform.GetSummaryFromTime(olive::core::rational(0),
									olive::core::rational(1, 2));
	ASSERT_EQ(summary.size(), 2);
	ExpectSummary(summary, 0, 0.8f, 0.8f);
	ExpectSummary(summary, 1, 0.8f, 0.8f);

	summary = waveform.GetSummaryFromTime(olive::core::rational(1, 2),
										  olive::core::rational(1, 2));
	ASSERT_EQ(summary.size(), 2);
	ExpectSummary(summary, 0, 0.0f, 0.0f);
	ExpectSummary(summary, 1, 0.0f, 0.0f);

	summary = waveform.GetSummaryFromTime(olive::core::rational(3, 2),
										  olive::core::rational(1, 2));
	ASSERT_EQ(summary.size(), 2);
	ExpectSummary(summary, 0, 0.8f, 0.8f);
	ExpectSummary(summary, 1, 0.8f, 0.8f);
}

TEST(AudioVisualWaveform, OverwriteSilenceExtendsLength)
{
	olive::AudioVisualWaveform waveform;
	waveform.set_channel_count(1);
	waveform.OverwriteSamples(MakeMonoConstant(size_t(kSampleRate), 0.5f),
							  kSampleRate, olive::core::rational(0));
	ASSERT_EQ(waveform.length(), olive::core::rational(1));

	// Silencing past the end grows the buffer with zeros
	waveform.OverwriteSilence(olive::core::rational(2),
							  olive::core::rational(1));

	EXPECT_EQ(waveform.length(), olive::core::rational(3));

	const olive::AudioVisualWaveform::Sample summary =
		waveform.GetSummaryFromTime(olive::core::rational(2),
									olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.0f, 0.0f);
}

// ---------------------------------------------------------------------------
// AudioVisualWaveform::Mid / Resize / TrimIn
// ---------------------------------------------------------------------------

TEST(AudioVisualWaveform, MidFromOffsetReturnsTail)
{
	olive::AudioVisualWaveform waveform;
	waveform.set_channel_count(1);
	waveform.OverwriteSamples(MakeSplitMonoBuffer(0.25f, 0.75f), kSampleRate,
							  olive::core::rational(0));

	const olive::AudioVisualWaveform mid =
		waveform.Mid(olive::core::rational(1));

	EXPECT_EQ(mid.length(), olive::core::rational(1));
	EXPECT_EQ(mid.channel_count(), 1);

	// The original is untouched
	EXPECT_EQ(waveform.length(), olive::core::rational(2));

	const olive::AudioVisualWaveform::Sample summary =
		mid.GetSummaryFromTime(olive::core::rational(1),
							   olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.75f, 0.75f);
}

TEST(AudioVisualWaveform, ResizeExtendPadsWithZeros)
{
	olive::AudioVisualWaveform waveform;
	waveform.set_channel_count(1);
	waveform.OverwriteSamples(MakeMonoConstant(size_t(kSampleRate), 0.5f),
							  kSampleRate, olive::core::rational(0));

	waveform.Resize(olive::core::rational(3));

	EXPECT_EQ(waveform.length(), olive::core::rational(3));

	olive::AudioVisualWaveform::Sample summary =
		waveform.GetSummaryFromTime(olive::core::rational(0),
									olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.5f, 0.5f);

	// The extended region is zero-filled
	summary = waveform.GetSummaryFromTime(olive::core::rational(2),
										  olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.0f, 0.0f);
}

TEST(AudioVisualWaveform, TrimInZeroIsNoOp)
{
	olive::AudioVisualWaveform waveform;
	waveform.set_channel_count(1);
	waveform.OverwriteSamples(MakeMonoConstant(size_t(kSampleRate), 0.5f),
							  kSampleRate, olive::core::rational(0));

	waveform.TrimIn(olive::core::rational(0));

	EXPECT_EQ(waveform.length(), olive::core::rational(1));

	const olive::AudioVisualWaveform::Sample summary =
		waveform.GetSummaryFromTime(olive::core::rational(0),
									olive::core::rational(1));
	ASSERT_EQ(summary.size(), 1);
	ExpectSummary(summary, 0, 0.5f, 0.5f);
}

// ---------------------------------------------------------------------------
// AudioVisualWaveform::SumSamples / ReSumSamples
// ---------------------------------------------------------------------------

TEST(AudioVisualWaveform, SumSamplesHonorsStartOffsetAndChannels)
{
	olive::core::SampleBuffer buffer(
		MakeParams(olive::core::kChannelLayoutStereo), size_t(100));
	float *left = buffer.data(0);
	float *right = buffer.data(1);
	for (size_t i = 0; i < buffer.sample_count(); i++) {
		left[i] = float(i) / 100.0f;
		right[i] = -float(i) / 100.0f;
	}

	// Summarize samples [10, 30) only
	const olive::AudioVisualWaveform::Sample summary =
		olive::AudioVisualWaveform::SumSamples(buffer, 10, 20);

	ASSERT_EQ(summary.size(), 2);
	// SumSamples (unlike ReSumSamples) reports the true extremes of the range
	EXPECT_FLOAT_EQ(summary.at(0).min, float(10) / 100.0f);
	EXPECT_FLOAT_EQ(summary.at(0).max, float(29) / 100.0f);
	EXPECT_FLOAT_EQ(summary.at(1).min, -float(29) / 100.0f);
	EXPECT_FLOAT_EQ(summary.at(1).max, -float(10) / 100.0f);
}

TEST(AudioVisualWaveform, ReSumSamplesMergesExtremesAcrossFrames)
{
	std::vector<olive::AudioVisualWaveform::SamplePerChannel> frames(4);
	frames[0] = { -0.2f, 0.3f }; // channel 0, narrow range
	frames[1] = { 0.0f, 0.1f }; // channel 1
	frames[2] = { -0.9f, 0.1f }; // channel 0, wider minimum
	frames[3] = { 0.05f, 0.08f }; // channel 1

	const olive::AudioVisualWaveform::Sample summary =
		olive::AudioVisualWaveform::ReSumSamples(frames.data(), 4, 2);

	ASSERT_EQ(summary.size(), 2);
	ExpectSummary(summary, 0, -0.9f, 0.3f);
	ExpectSummary(summary, 1, 0.0f, 0.1f);
}

TEST(AudioVisualWaveform, ReSumSamplesSingleFrameIsIdentity)
{
	std::vector<olive::AudioVisualWaveform::SamplePerChannel> frames(2);
	frames[0] = { -0.3f, 0.7f };
	frames[1] = { -0.1f, 0.2f };

	const olive::AudioVisualWaveform::Sample summary =
		olive::AudioVisualWaveform::ReSumSamples(frames.data(), 2, 2);

	ASSERT_EQ(summary.size(), 2);
	ExpectSummary(summary, 0, -0.3f, 0.7f);
	ExpectSummary(summary, 1, -0.1f, 0.2f);
}

// ---------------------------------------------------------------------------
// AudioVisualWaveform::DrawSample
// ---------------------------------------------------------------------------

TEST(AudioVisualWaveform, DrawSamplePaintsVerticalSpan)
{
	QImage image(4, 100, QImage::Format_ARGB32);
	image.fill(Qt::transparent);

	{
		QPainter painter(&image);
		const olive::AudioVisualWaveform::Sample sample = { { -1.0f, 1.0f } };
		olive::AudioVisualWaveform::DrawSample(&painter, sample, 1, 0, 100,
											   false);
	}

	// A full-scale sample spans the whole column
	EXPECT_GT(image.pixelColor(1, 10).alpha(), 0);
	EXPECT_GT(image.pixelColor(1, 90).alpha(), 0);
}

TEST(AudioVisualWaveform, DrawSampleIgnoresEmptySample)
{
	QImage image(4, 100, QImage::Format_ARGB32);
	image.fill(Qt::transparent);

	{
		QPainter painter(&image);
		olive::AudioVisualWaveform::DrawSample(
			&painter, olive::AudioVisualWaveform::Sample(), 1, 0, 100, false);
	}

	EXPECT_EQ(image.pixelColor(1, 10).alpha(), 0);
}

// ---------------------------------------------------------------------------
// AudioProcessor
// ---------------------------------------------------------------------------

TEST(AudioProcessor, ConvertPassthroughCopiesInputSamples)
{
	olive::AudioProcessor processor;
	const olive::core::AudioParams params =
		MakeParams(olive::core::kChannelLayoutStereo);
	ASSERT_TRUE(processor.Open(params, params, 1.0));

	constexpr int kSamples = 1024;
	std::vector<float> left(kSamples, 0.5f);
	std::vector<float> right(kSamples, -0.25f);
	float *input[2] = { left.data(), right.data() };

	olive::AudioProcessor::Buffer output;
	EXPECT_EQ(processor.Convert(input, kSamples, &output), 0);

	// Planar output keeps one byte plane per channel
	ASSERT_EQ(output.size(), 2);
	ASSERT_EQ(output.at(0).size(), kSamples * int(sizeof(float)));
	ASSERT_EQ(output.at(1).size(), kSamples * int(sizeof(float)));

	float value = 0.0f;
	std::memcpy(&value, output.at(0).constData(), sizeof(float));
	EXPECT_FLOAT_EQ(value, 0.5f);
	std::memcpy(&value,
				output.at(0).constData() + (kSamples - 1) * sizeof(float),
				sizeof(float));
	EXPECT_FLOAT_EQ(value, 0.5f);
	std::memcpy(&value, output.at(1).constData(), sizeof(float));
	EXPECT_FLOAT_EQ(value, -0.25f);
}

TEST(AudioProcessor, ConvertToPackedInterleavesChannels)
{
	olive::AudioProcessor processor;
	const olive::core::AudioParams from =
		MakeParams(olive::core::kChannelLayoutStereo);
	const olive::core::AudioParams to(kSampleRate,
									  olive::core::kChannelLayoutStereo,
									  olive::core::SampleFormat::F32);
	ASSERT_TRUE(processor.Open(from, to, 1.0));

	constexpr int kSamples = 1024;
	std::vector<float> left(kSamples, 0.5f);
	std::vector<float> right(kSamples, -0.25f);
	float *input[2] = { left.data(), right.data() };

	olive::AudioProcessor::Buffer output;
	EXPECT_EQ(processor.Convert(input, kSamples, &output), 0);

	// Packed output folds both channels into a single interleaved plane
	ASSERT_EQ(output.size(), 1);
	ASSERT_EQ(output.at(0).size(), kSamples * 2 * int(sizeof(float)));

	float left_value = 0.0f;
	float right_value = 0.0f;
	std::memcpy(&left_value, output.at(0).constData(), sizeof(float));
	std::memcpy(&right_value, output.at(0).constData() + sizeof(float),
				sizeof(float));
	EXPECT_FLOAT_EQ(left_value, 0.5f);
	EXPECT_FLOAT_EQ(right_value, -0.25f);
}

TEST(AudioProcessor, ConvertDownmixToMonoReducesPlaneCount)
{
	olive::AudioProcessor processor;
	ASSERT_TRUE(processor.Open(MakeParams(olive::core::kChannelLayoutStereo),
							   MakeParams(olive::core::kChannelLayoutMono),
							   1.0));

	constexpr int kSamples = 1024;
	std::vector<float> left(kSamples, 0.5f);
	std::vector<float> right(kSamples, 0.5f);
	float *input[2] = { left.data(), right.data() };

	olive::AudioProcessor::Buffer output;
	EXPECT_EQ(processor.Convert(input, kSamples, &output), 0);

	ASSERT_EQ(output.size(), 1);
	ASSERT_EQ(output.at(0).size(), kSamples * int(sizeof(float)));

	// The downmix of two identical channels must stay audible regardless of
	// the exact mixing coefficients
	float value = 0.0f;
	std::memcpy(&value, output.at(0).constData(), sizeof(float));
	EXPECT_GT(value, 0.0f);
	EXPECT_LE(value, 1.0f);
}

TEST(AudioProcessor, ConvertResampleDrainProducesExpectedSampleCount)
{
	olive::AudioProcessor processor;
	const olive::core::AudioParams from =
		MakeParams(olive::core::kChannelLayoutStereo);
	const olive::core::AudioParams to(kSampleRate / 2,
									  olive::core::kChannelLayoutStereo,
									  olive::core::SampleFormat::F32P);
	ASSERT_TRUE(processor.Open(from, to, 1.0));

	// One second of input
	constexpr int kSamples = 48000;
	std::vector<float> left(kSamples, 0.5f);
	std::vector<float> right(kSamples, 0.5f);
	float *input[2] = { left.data(), right.data() };

	const olive::AudioProcessor::Buffer output =
		ConvertAndDrain(processor, input, kSamples);

	ASSERT_EQ(output.size(), 2);
	EXPECT_EQ(output.at(0).size(), output.at(1).size());

	// 48 kHz downsampled to 24 kHz must produce about half the samples; the
	// resampler's filter delay makes the exact total version-dependent
	const int converted = output.at(0).size() / int(sizeof(float));
	EXPECT_GE(converted, 23000);
	EXPECT_LE(converted, 24500);
}

TEST(AudioProcessor, ConvertTempoDrainReducesSampleCount)
{
	olive::AudioProcessor processor;
	const olive::core::AudioParams params =
		MakeParams(olive::core::kChannelLayoutStereo);
	ASSERT_TRUE(processor.Open(params, params, 2.0));

	// One second of input
	constexpr int kSamples = 48000;
	std::vector<float> left(kSamples, 0.5f);
	std::vector<float> right(kSamples, 0.5f);
	float *input[2] = { left.data(), right.data() };

	const olive::AudioProcessor::Buffer output =
		ConvertAndDrain(processor, input, kSamples);

	ASSERT_EQ(output.size(), 2);

	// 2x tempo must output roughly half the input; atempo works in windows,
	// so allow generous margins
	const int converted = output.at(0).size() / int(sizeof(float));
	EXPECT_GE(converted, 20000);
	EXPECT_LE(converted, 28000);
}

TEST(AudioProcessor, ConvertWithNullOutputOnlyPushes)
{
	olive::AudioProcessor processor;
	const olive::core::AudioParams params =
		MakeParams(olive::core::kChannelLayoutStereo);
	ASSERT_TRUE(processor.Open(params, params, 1.0));

	constexpr int kSamples = 1024;
	std::vector<float> left(kSamples, 0.5f);
	std::vector<float> right(kSamples, 0.5f);
	float *input[2] = { left.data(), right.data() };

	// A null output buffer means push-only and is not an error
	EXPECT_EQ(processor.Convert(input, kSamples, nullptr), 0);
}

TEST(AudioProcessor, ConvertWithNoInputReturnsZeroWithEmptyPlanes)
{
	olive::AudioProcessor processor;
	const olive::core::AudioParams params =
		MakeParams(olive::core::kChannelLayoutStereo);
	ASSERT_TRUE(processor.Open(params, params, 1.0));

	olive::AudioProcessor::Buffer output;
	EXPECT_EQ(processor.Convert(nullptr, 0, &output), 0);

	// The output is still sized to the planar channel count, but empty
	ASSERT_EQ(output.size(), 2);
	EXPECT_TRUE(output.at(0).isEmpty());
	EXPECT_TRUE(output.at(1).isEmpty());
}

TEST(AudioProcessor, OpenFixesZeroChannelLayout)
{
	olive::AudioProcessor processor;

	// A zero layout mask is unusable by the filter graph and must be replaced
	// with a default layout derived from the channel count
	const olive::core::AudioParams from(kSampleRate, 0,
										olive::core::SampleFormat::F32P);
	const olive::core::AudioParams to =
		MakeParams(olive::core::kChannelLayoutStereo);

	ASSERT_TRUE(processor.Open(from, to, 1.0));
	EXPECT_NE(processor.from().channel_layout(), uint64_t(0));
	EXPECT_EQ(processor.from().channel_count(), 2);
}

TEST(AudioProcessor, CloseIsIdempotentAndReopenSucceeds)
{
	olive::AudioProcessor processor;
	const olive::core::AudioParams params =
		MakeParams(olive::core::kChannelLayoutStereo);

	// Closing an unopened processor must be safe
	processor.Close();
	EXPECT_FALSE(processor.IsOpen());

	ASSERT_TRUE(processor.Open(params, params, 1.0));
	processor.Close();
	processor.Close();
	EXPECT_FALSE(processor.IsOpen());

	EXPECT_TRUE(processor.Open(params, params, 1.0));
	EXPECT_TRUE(processor.IsOpen());
}

TEST(AudioProcessor, FlushWithoutOpenDoesNotCrash)
{
	olive::AudioProcessor processor;

	// Logs an error but must not crash
	processor.Flush();
	EXPECT_FALSE(processor.IsOpen());
}
