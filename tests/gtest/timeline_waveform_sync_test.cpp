/***
  Oak Video Editor - Regression test for TimelineWidget waveform sync
  Copyright (C) 2026 Oak Team
***/

#include <gtest/gtest.h>

#include <cmath>

#include "audio/audiovisualwaveform.h"
#include "node/block/clip/clip.h"
#include "node/node.h"
#include "node/project/footage/footage.h"
#include "olive/core/render/audioparams.h"
#include "olive/core/render/samplebuffer.h"
#include "olive/core/render/sampleformat.h"
#include "render/audiowaveformcache.h"
#include "widget/timelinewidget/timelinewidgetwaveformsync.h"

using namespace olive;
using namespace olive::core;

namespace
{

AudioParams MakeMonoParams(int sample_rate)
{
	return AudioParams(sample_rate, static_cast<uint64_t>(kChannelLayoutMono),
					   SampleFormat::F32P);
}

SampleBuffer MakeMonoBuffer(int sample_rate, float value, int seconds)
{
	SampleBuffer buf(MakeMonoParams(sample_rate),
					 static_cast<size_t>(sample_rate * seconds));
	float *data = buf.data(0);
	for (size_t i = 0; i < buf.sample_count(); i++) {
		data[i] = value;
	}
	return buf;
}

void WritePartialWaveform(AudioWaveformCache *cache, int sample_rate)
{
	const AudioParams params = MakeMonoParams(sample_rate);
	cache->SetParameters(params);

	// Fill seconds [1,2) with a loud constant signal.
	AudioVisualWaveform waveform;
	waveform.set_channel_count(1);
	SampleBuffer buf = MakeMonoBuffer(sample_rate, 1.0f, 1);
	waveform.OverwriteSamples(buf, sample_rate, rational(1));

	// Tell the cache that only the middle second is valid in a 3-second clip.
	cache->WriteWaveform(TimeRange(1, 2), TimeRangeList({ TimeRange(1, 2) }),
						 &waveform);
}

} // namespace

TEST(TimelineWaveformSync, ExtractEnvelopeUsesOnlyValidatedRanges)
{
	constexpr int kSampleRate = 48000;
	constexpr size_t kWindowSamples = kSampleRate / 20; // 50 ms windows

	AudioWaveformCache cache;
	WritePartialWaveform(&cache, kSampleRate);

	WaveformSyncClip clip;
	clip.waveform = &cache;
	clip.media_range = TimeRange(0, 3);
	clip.sample_rate = kSampleRate;

	const QVector<double> envelope =
		TimelineWaveformSync::ExtractWaveformCacheEnvelope(clip, kSampleRate,
														   kWindowSamples);

	// 3 seconds at 20 windows per second == 60 windows.
	EXPECT_EQ(envelope.size(), 60);

	// Window before the validated region should be silent.
	EXPECT_DOUBLE_EQ(envelope.at(0), 0.0);

	// Windows inside the validated region should have a non-zero peak.
	bool found_nonzero = false;
	for (int i = 20; i < 40; ++i) {
		if (envelope.at(i) > 0.0) {
			found_nonzero = true;
			break;
		}
	}
	EXPECT_TRUE(found_nonzero);

	// Window after the validated region should also be silent.
	EXPECT_DOUBLE_EQ(envelope.at(59), 0.0);
}

TEST(TimelineWaveformSync, ExtractEnvelopeReportsValidityMask)
{
	constexpr int kSampleRate = 48000;
	constexpr size_t kWindowSamples = kSampleRate / 20; // 50 ms windows

	AudioWaveformCache cache;
	WritePartialWaveform(&cache, kSampleRate);

	WaveformSyncClip clip;
	clip.waveform = &cache;
	clip.media_range = TimeRange(0, 3);
	clip.sample_rate = kSampleRate;

	QVector<bool> valid_mask;
	const QVector<double> envelope =
		TimelineWaveformSync::ExtractWaveformCacheEnvelope(
			clip, kSampleRate, kWindowSamples, &valid_mask);

	// One flag per envelope window
	ASSERT_EQ(valid_mask.size(), envelope.size());

	// Windows outside the validated second are flagged invalid, windows
	// inside it are flagged valid
	EXPECT_FALSE(valid_mask.at(0));
	EXPECT_FALSE(valid_mask.at(59));
	for (int i = 20; i < 40; ++i) {
		EXPECT_TRUE(valid_mask.at(i));
	}
}

TEST(TimelineWaveformSync, PartialCacheIsConsideredReady)
{
	constexpr int kSampleRate = 48000;

	Footage footage;
	footage.SetValid();

	AudioWaveformCache *cache = footage.waveform_cache();
	WritePartialWaveform(cache, kSampleRate);

	ClipBlock clip;
	clip.set_length_and_media_out(rational(3));
	clip.set_media_in(rational(0));

	Node::ConnectEdge(&footage, NodeInput(&clip, ClipBlock::kBufferIn));

	WaveformSyncClip out;
	EXPECT_TRUE(TimelineWaveformSync::GetWaveformSyncClip(&clip, &out));
	EXPECT_EQ(out.waveform, cache);
	EXPECT_EQ(out.sample_rate, kSampleRate);
	EXPECT_EQ(out.media_range, TimeRange(0, 3));
}

TEST(TimelineWaveformSync, EmptyCacheIsNotReady)
{
	Footage footage;
	footage.SetValid();

	AudioParams params = MakeMonoParams(48000);
	footage.waveform_cache()->SetParameters(params);

	ClipBlock clip;
	clip.set_length_and_media_out(rational(3));
	clip.set_media_in(rational(0));

	Node::ConnectEdge(&footage, NodeInput(&clip, ClipBlock::kBufferIn));

	WaveformSyncClip out;
	EXPECT_FALSE(TimelineWaveformSync::GetWaveformSyncClip(&clip, &out));
}
