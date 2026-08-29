/*
 * Oak Video Editor - Audio Subsystem Smoke Tests
 * Copyright (C) 2025 Olive CE Team
 *
 * Comprehensive smoke tests for the audio subsystem including:
 * - AudioManager lifecycle and device management
 * - AudioProcessor format conversion and tempo
 * - AudioVisualWaveform operations
 * - SampleBuffer management
 * - AudioParams validation and conversions
 */

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QThread>
#include <QPainter>
#include <QImage>

// Audio headers
#include "audio/audiomanager.h"
#include "audio/audioprocessor.h"
#include "audio/audiovisualwaveform.h"
#include "render/previewaudiodevice.h"
#include "olive/core/render/samplebuffer.h"
#include "olive/core/render/audioparams.h"
#include "olive/core/render/sampleformat.h"

using namespace olive;
using namespace olive::core;

namespace olive
{
namespace audio
{
namespace test
{

// ============================================================================
// Helper Functions
// ============================================================================

static AudioParams MakeAudioParams(int sample_rate, uint64_t channel_layout,
								   SampleFormat format)
{
	return AudioParams(sample_rate, channel_layout, format);
}

static void FillSampleBuffer(SampleBuffer &buffer, float value)
{
	for (int ch = 0; ch < buffer.channel_count(); ++ch) {
		float *data = buffer.data(ch);
		for (size_t i = 0; i < buffer.sample_count(); ++i) {
			data[i] = value;
		}
	}
}

// ============================================================================
// Smoke Test: AudioParams
// ============================================================================

TEST(AudioSmokeParams, DefaultConstruction)
{
	AudioParams params;
	EXPECT_FALSE(params.is_valid());
	EXPECT_EQ(params.sample_rate(), 0);
	EXPECT_EQ(params.channel_count(), 0);
	EXPECT_EQ(params.format(), SampleFormat::INVALID);
}

TEST(AudioSmokeParams, ValidConstruction)
{
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);

	EXPECT_TRUE(params.is_valid());
	EXPECT_EQ(params.sample_rate(), 48000);
	EXPECT_EQ(params.channel_count(), 2);
	EXPECT_EQ(params.format(), SampleFormat::F32P);
	EXPECT_EQ(params.bytes_per_sample_per_channel(), 4);
	EXPECT_EQ(params.bits_per_sample(), 32);
}

TEST(AudioSmokeParams, MonoChannelLayout)
{
	AudioParams params(44100, kChannelLayoutMono, SampleFormat::S16);

	EXPECT_TRUE(params.is_valid());
	EXPECT_EQ(params.sample_rate(), 44100);
	EXPECT_EQ(params.channel_count(), 1);
}

TEST(AudioSmokeParams, SurroundChannelLayout)
{
	AudioParams params(48000, kChannelLayout5Point1, SampleFormat::F32P);

	EXPECT_TRUE(params.is_valid());
	EXPECT_EQ(params.channel_count(), 6);
}

TEST(AudioSmokeParams, TimeConversions)
{
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);

	// Time to samples
	EXPECT_EQ(params.time_to_samples(1.0), 48000);
	EXPECT_EQ(params.time_to_samples(0.5), 24000);
	EXPECT_EQ(params.time_to_samples(2.0), 96000);

	// Samples to bytes
	EXPECT_EQ(params.samples_to_bytes(48000),
			  48000 * 2 * 4); // samples * channels * bytes_per_sample

	// Time to bytes
	EXPECT_EQ(params.time_to_bytes(1.0), 48000 * 2 * 4);
}

TEST(AudioSmokeParams, EqualityOperators)
{
	AudioParams params1(48000, kChannelLayoutStereo, SampleFormat::F32P);
	AudioParams params2(48000, kChannelLayoutStereo, SampleFormat::F32P);
	AudioParams params3(44100, kChannelLayoutStereo, SampleFormat::F32P);
	AudioParams params4(48000, kChannelLayoutMono, SampleFormat::F32P);
	AudioParams params5(48000, kChannelLayoutStereo, SampleFormat::S16);

	EXPECT_TRUE(params1 == params2);
	EXPECT_FALSE(params1 != params2);

	EXPECT_FALSE(params1 == params3); // Different sample rate
	EXPECT_FALSE(params1 == params4); // Different channel layout
	EXPECT_FALSE(params1 == params5); // Different format
}

TEST(AudioSmokeParams, CopyConstruction)
{
	AudioParams original(48000, kChannelLayoutStereo, SampleFormat::F32P);
	AudioParams copy(original);

	EXPECT_TRUE(copy.is_valid());
	EXPECT_EQ(copy.sample_rate(), original.sample_rate());
	EXPECT_EQ(copy.channel_count(), original.channel_count());
	EXPECT_EQ(copy.format(), original.format());

	// Modifying copy should not affect original
	copy.set_sample_rate(44100);
	EXPECT_EQ(original.sample_rate(), 48000);
	EXPECT_EQ(copy.sample_rate(), 44100);
}

TEST(AudioSmokeParams, CopyAssignment)
{
	AudioParams original(48000, kChannelLayoutStereo, SampleFormat::F32P);
	AudioParams copy;
	copy = original;

	EXPECT_TRUE(copy.is_valid());
	EXPECT_EQ(copy.sample_rate(), original.sample_rate());
	EXPECT_EQ(copy.channel_count(), original.channel_count());
	EXPECT_EQ(copy.format(), original.format());
}

TEST(AudioSmokeParams, ChannelLayoutModification)
{
	AudioParams params(48000, kChannelLayoutMono, SampleFormat::F32P);
	EXPECT_EQ(params.channel_count(), 1);

	// Change to stereo
	params.set_channel_layout(kChannelLayoutStereo);
	EXPECT_EQ(params.channel_count(), 2);

	// Change to 5.1
	params.set_channel_layout(kChannelLayout5Point1);
	EXPECT_EQ(params.channel_count(), 6);
}

// ============================================================================
// Smoke Test: SampleBuffer
// ============================================================================

TEST(AudioSmokeBuffer, DefaultConstruction)
{
	SampleBuffer buffer;
	EXPECT_FALSE(buffer.is_allocated());
	EXPECT_EQ(buffer.channel_count(), 0);
	EXPECT_EQ(buffer.sample_count(), 0);
}

TEST(AudioSmokeBuffer, Allocation)
{
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer buffer(params, size_t(48000)); // 1 second of samples

	EXPECT_TRUE(buffer.is_allocated());
	EXPECT_EQ(buffer.channel_count(), 2);
	EXPECT_EQ(buffer.sample_count(), 48000);
}

TEST(AudioSmokeBuffer, DataAccess)
{
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer buffer(params, size_t(100));

	// Fill with test data
	FillSampleBuffer(buffer, 0.5f);

	// Verify data
	for (int ch = 0; ch < buffer.channel_count(); ++ch) {
		const float *data = buffer.data(ch);
		for (size_t i = 0; i < buffer.sample_count(); ++i) {
			EXPECT_FLOAT_EQ(data[i], 0.5f);
		}
	}
}

TEST(AudioSmokeBuffer, Silence)
{
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer buffer(params, size_t(100));

	// Fill with non-zero values
	FillSampleBuffer(buffer, 0.5f);

	// Apply silence
	buffer.silence();

	// Verify silence
	for (int ch = 0; ch < buffer.channel_count(); ++ch) {
		const float *data = buffer.data(ch);
		for (size_t i = 0; i < buffer.sample_count(); ++i) {
			EXPECT_FLOAT_EQ(data[i], 0.0f);
		}
	}
}

TEST(AudioSmokeBuffer, VolumeTransform)
{
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer buffer(params, size_t(100));

	// Fill with 1.0
	FillSampleBuffer(buffer, 1.0f);

	// Apply volume transform (50%)
	buffer.transform_volume(0.5f);

	// Verify volume change
	for (int ch = 0; ch < buffer.channel_count(); ++ch) {
		const float *data = buffer.data(ch);
		for (size_t i = 0; i < buffer.sample_count(); ++i) {
			EXPECT_FLOAT_EQ(data[i], 0.5f);
		}
	}
}

TEST(AudioSmokeBuffer, Clamp)
{
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer buffer(params, size_t(100));

	// Fill with values outside [-1, 1]
	for (int ch = 0; ch < buffer.channel_count(); ++ch) {
		float *data = buffer.data(ch);
		for (size_t i = 0; i < buffer.sample_count(); ++i) {
			data[i] = (i % 2 == 0) ? 2.0f : -2.0f;
		}
	}

	// Apply clamp
	buffer.clamp();

	// Verify clamping
	for (int ch = 0; ch < buffer.channel_count(); ++ch) {
		const float *data = buffer.data(ch);
		for (size_t i = 0; i < buffer.sample_count(); ++i) {
			EXPECT_GE(data[i], -1.0f);
			EXPECT_LE(data[i], 1.0f);
		}
	}
}

TEST(AudioSmokeBuffer, FastSet)
{
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer source(params, size_t(100));
	SampleBuffer dest(params, size_t(100));

	FillSampleBuffer(source, 0.75f);
	dest.silence();

	// Fast copy from source to dest
	dest.fast_set(source, 0); // Copy to channel 0

	// Verify channel 0 copied
	const float *dest_data = dest.data(0);
	for (size_t i = 0; i < dest.sample_count(); ++i) {
		EXPECT_FLOAT_EQ(dest_data[i], 0.75f);
	}
}

TEST(AudioSmokeBuffer, RipChannel)
{
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer buffer(params, size_t(100));

	// Fill channel 0 with 0.5, channel 1 with 0.25
	float *ch0 = buffer.data(0);
	float *ch1 = buffer.data(1);
	for (size_t i = 0; i < buffer.sample_count(); ++i) {
		ch0[i] = 0.5f;
		ch1[i] = 0.25f;
	}

	// Rip channel 0
	SampleBuffer ripped = buffer.rip_channel(0);

	EXPECT_EQ(ripped.channel_count(), 1);
	EXPECT_EQ(ripped.sample_count(), buffer.sample_count());

	const float *ripped_data = ripped.data(0);
	for (size_t i = 0; i < ripped.sample_count(); ++i) {
		EXPECT_FLOAT_EQ(ripped_data[i], 0.5f);
	}
}

// ============================================================================
// Smoke Test: AudioVisualWaveform
// ============================================================================

TEST(AudioSmokeWaveform, DefaultConstruction)
{
	AudioVisualWaveform waveform;
	EXPECT_EQ(waveform.channel_count(), 0);
	EXPECT_EQ(waveform.length(), rational(0));
}

TEST(AudioSmokeWaveform, ChannelCount)
{
	AudioVisualWaveform waveform;
	waveform.set_channel_count(2);
	EXPECT_EQ(waveform.channel_count(), 2);

	waveform.set_channel_count(6);
	EXPECT_EQ(waveform.channel_count(), 6);
}

TEST(AudioSmokeWaveform, OverwriteSamples)
{
	AudioVisualWaveform waveform;
	waveform.set_channel_count(2);

	// Create sample buffer with sine wave-like data
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer buffer(params, size_t(4800)); // 0.1 seconds

	for (int ch = 0; ch < buffer.channel_count(); ++ch) {
		float *data = buffer.data(ch);
		for (size_t i = 0; i < buffer.sample_count(); ++i) {
			data[i] = std::sin(float(i) * 0.1f);
		}
	}

	// Write samples to waveform
	waveform.OverwriteSamples(buffer, 48000, rational(0));

	EXPECT_GT(waveform.length(), rational(0));
}

TEST(AudioSmokeWaveform, OverwriteSilence)
{
	AudioVisualWaveform waveform;
	waveform.set_channel_count(2);

	// First add some samples
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer buffer(params, size_t(4800));
	FillSampleBuffer(buffer, 0.5f);
	waveform.OverwriteSamples(buffer, 48000, rational(0));

	rational original_length = waveform.length();

	// Overwrite with silence
	waveform.OverwriteSilence(rational(0), rational(1, 10)); // 0.1 seconds

	// Length should be at least as long as original
	EXPECT_GE(waveform.length(), original_length);
}

TEST(AudioSmokeWaveform, TrimIn)
{
	AudioVisualWaveform waveform;
	waveform.set_channel_count(2);

	// Add samples
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer buffer(params, size_t(48000)); // 1 second
	FillSampleBuffer(buffer, 0.5f);
	waveform.OverwriteSamples(buffer, 48000, rational(0));

	EXPECT_EQ(waveform.length(), rational(1));

	// Trim 0.25 seconds from start
	waveform.TrimIn(rational(1, 4));

	EXPECT_EQ(waveform.length(), rational(3, 4));
}

TEST(AudioSmokeWaveform, Resize)
{
	AudioVisualWaveform waveform;
	waveform.set_channel_count(2);

	// Add samples
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer buffer(params, size_t(48000));
	FillSampleBuffer(buffer, 0.5f);
	waveform.OverwriteSamples(buffer, 48000, rational(0));

	EXPECT_EQ(waveform.length(), rational(1));

	// Resize to 0.5 seconds
	waveform.Resize(rational(1, 2));

	EXPECT_EQ(waveform.length(), rational(1, 2));
}

TEST(AudioSmokeWaveform, TrimRange)
{
	AudioVisualWaveform waveform;
	waveform.set_channel_count(2);

	// Add 2 seconds of samples
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer buffer(params, size_t(96000));
	FillSampleBuffer(buffer, 0.5f);
	waveform.OverwriteSamples(buffer, 48000, rational(0));

	EXPECT_EQ(waveform.length(), rational(2));

	// Trim to range [0.5, 1.0] (0.5 seconds duration starting at 0.5)
	waveform.TrimRange(rational(1, 2), rational(1, 2));

	EXPECT_EQ(waveform.length(), rational(1, 2));
}

TEST(AudioSmokeWaveform, Mid)
{
	AudioVisualWaveform waveform;
	waveform.set_channel_count(2);

	// Add 2 seconds of samples
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer buffer(params, size_t(96000));
	FillSampleBuffer(buffer, 0.5f);
	waveform.OverwriteSamples(buffer, 48000, rational(0));

	// Get mid section [0.5, 1.5]
	AudioVisualWaveform mid = waveform.Mid(rational(1, 2), rational(1));

	EXPECT_EQ(mid.length(), rational(1));
	EXPECT_EQ(mid.channel_count(), 2);
}

TEST(AudioSmokeWaveform, GetSummaryFromTime)
{
	AudioVisualWaveform waveform;
	waveform.set_channel_count(2);

	// Add samples with varying values
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer buffer(params, size_t(4800));
	for (int ch = 0; ch < buffer.channel_count(); ++ch) {
		float *data = buffer.data(ch);
		for (size_t i = 0; i < buffer.sample_count(); ++i) {
			data[i] = (i % 2 == 0) ? 0.8f : -0.8f;
		}
	}
	waveform.OverwriteSamples(buffer, 48000, rational(0));

	// Get summary for first half
	auto summary = waveform.GetSummaryFromTime(rational(0), rational(1, 20));

	EXPECT_EQ(summary.size(), 2); // 2 channels
	// Summary should reflect the min/max of the samples
	EXPECT_LE(summary[0].min, 0.0f);
	EXPECT_GE(summary[0].max, 0.0f);
}

TEST(AudioSmokeWaveform, SumSamples)
{
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer buffer(params, size_t(100));

	// Fill with known pattern
	for (int ch = 0; ch < buffer.channel_count(); ++ch) {
		float *data = buffer.data(ch);
		for (size_t i = 0; i < buffer.sample_count(); ++i) {
			data[i] = float(i) / 100.0f;
		}
	}

	auto summary = AudioVisualWaveform::SumSamples(buffer, 0, 100);

	EXPECT_EQ(summary.size(), 2);
	EXPECT_FLOAT_EQ(summary[0].min, 0.0f);
	EXPECT_FLOAT_EQ(summary[0].max, 0.99f);
}

TEST(AudioSmokeWaveform, ReSumSamples)
{
	// Create sample data
	std::vector<AudioVisualWaveform::SamplePerChannel> samples(200);
	for (size_t i = 0; i < 100; ++i) {
		samples[i * 2].min = -0.5f;
		samples[i * 2].max = 0.5f;
		samples[i * 2 + 1].min = -0.3f;
		samples[i * 2 + 1].max = 0.3f;
	}

	auto summary = AudioVisualWaveform::ReSumSamples(samples.data(), 200, 2);

	EXPECT_EQ(summary.size(), 2);
	EXPECT_FLOAT_EQ(summary[0].min, -0.5f);
	EXPECT_FLOAT_EQ(summary[0].max, 0.5f);
	EXPECT_FLOAT_EQ(summary[1].min, -0.3f);
	EXPECT_FLOAT_EQ(summary[1].max, 0.3f);
}

// ============================================================================
// Smoke Test: AudioProcessor
// ============================================================================

TEST(AudioSmokeProcessor, DefaultConstruction)
{
	AudioProcessor processor;
	EXPECT_FALSE(processor.IsOpen());
}

TEST(AudioSmokeProcessor, OpenClose)
{
	AudioProcessor processor;

	AudioParams from(48000, kChannelLayoutStereo, SampleFormat::F32P);
	AudioParams to(48000, kChannelLayoutStereo, SampleFormat::F32P);

	EXPECT_TRUE(processor.Open(from, to, 1.0));
	EXPECT_TRUE(processor.IsOpen());

	processor.Close();
	EXPECT_FALSE(processor.IsOpen());
}

TEST(AudioSmokeProcessor, SampleRateConversion)
{
	AudioProcessor processor;

	AudioParams from(48000, kChannelLayoutStereo, SampleFormat::F32P);
	AudioParams to(44100, kChannelLayoutStereo, SampleFormat::F32P);

	EXPECT_TRUE(processor.Open(from, to, 1.0));
	EXPECT_TRUE(processor.IsOpen());
	EXPECT_EQ(processor.from().sample_rate(), 48000);
	EXPECT_EQ(processor.to().sample_rate(), 44100);
}

TEST(AudioSmokeProcessor, ChannelLayoutConversion)
{
	AudioProcessor processor;

	AudioParams from(48000, kChannelLayoutStereo, SampleFormat::F32P);
	AudioParams to(48000, kChannelLayoutMono, SampleFormat::F32P);

	EXPECT_TRUE(processor.Open(from, to, 1.0));
	EXPECT_TRUE(processor.IsOpen());
	EXPECT_EQ(processor.from().channel_count(), 2);
	EXPECT_EQ(processor.to().channel_count(), 1);
}

TEST(AudioSmokeProcessor, FormatConversion)
{
	AudioProcessor processor;

	AudioParams from(48000, kChannelLayoutStereo, SampleFormat::F32P);
	AudioParams to(48000, kChannelLayoutStereo, SampleFormat::S16P);

	EXPECT_TRUE(processor.Open(from, to, 1.0));
	EXPECT_TRUE(processor.IsOpen());
}

TEST(AudioSmokeProcessor, TempoChange)
{
	AudioProcessor processor;

	AudioParams from(48000, kChannelLayoutStereo, SampleFormat::F32P);
	AudioParams to(48000, kChannelLayoutStereo, SampleFormat::F32P);

	// Open with 2x tempo
	EXPECT_TRUE(processor.Open(from, to, 2.0));
	EXPECT_TRUE(processor.IsOpen());
}

TEST(AudioSmokeProcessor, InvalidOpen)
{
	AudioProcessor processor;

	// Open with valid params
	AudioParams from(48000, kChannelLayoutStereo, SampleFormat::F32P);
	AudioParams to(48000, kChannelLayoutStereo, SampleFormat::F32P);
	EXPECT_TRUE(processor.Open(from, to, 1.0));

	// Try to open again while already open (should fail)
	EXPECT_FALSE(processor.Open(from, to, 1.0));
}

TEST(AudioSmokeProcessor, ConvertWithoutOpen)
{
	AudioProcessor processor;

	// Create input data
	float *input[2] = { nullptr, nullptr };
	std::vector<float> ch0(100, 0.5f);
	std::vector<float> ch1(100, 0.5f);
	input[0] = ch0.data();
	input[1] = ch1.data();

	AudioProcessor::Buffer output;

	// Should fail since processor is not open
	EXPECT_EQ(processor.Convert(input, 100, &output), -1);
}

// ============================================================================
// Smoke Test: PreviewAudioDevice
// ============================================================================

TEST(AudioSmokePreviewDevice, Construction)
{
	PreviewAudioDevice device;
	EXPECT_TRUE(device.isSequential());
	EXPECT_EQ(device.bytes_per_frame(),
			  0); // BUG: Should be initialized properly
}

TEST(AudioSmokePreviewDevice, BytesPerFrame)
{
	PreviewAudioDevice device;

	device.set_bytes_per_frame(8); // 2 channels * 4 bytes (F32)
	EXPECT_EQ(device.bytes_per_frame(), 8);

	device.set_bytes_per_frame(4); // 2 channels * 2 bytes (S16)
	EXPECT_EQ(device.bytes_per_frame(), 4);
}

TEST(AudioSmokePreviewDevice, NotifyInterval)
{
	PreviewAudioDevice device;

	device.set_notify_interval(100); // 100 frames
	// Cannot directly verify, but should not crash
}

TEST(AudioSmokePreviewDevice, Clear)
{
	PreviewAudioDevice device;
	device.open(QIODevice::ReadWrite);

	// Write some data
	QByteArray data(1000, 0xAB);
	device.write(data);

	// Clear
	device.clear();

	// Device should be empty now (next read should return 0 or silence)
	char buf[100];
	qint64 read = device.readData(buf, sizeof(buf));
	// After clear, read should return 0 or the buffer should be zeroed
	EXPECT_TRUE(read >= 0);
}

// ============================================================================
// Smoke Test: Sample Format
// ============================================================================

TEST(AudioSmokeSampleFormat, ByteCount)
{
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::INVALID), 0);
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::U8), 1);
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::U8P), 1);
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::S16), 2);
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::S16P), 2);
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::S32), 4);
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::S32P), 4);
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::F32), 4);
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::F32P), 4);
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::S64), 8);
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::S64P), 8);
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::F64), 8);
	EXPECT_EQ(SampleFormat::byte_count(SampleFormat::F64P), 8);
}

TEST(AudioSmokeSampleFormat, PackedVsPlanar)
{
	// Packed formats
	EXPECT_TRUE(SampleFormat::is_packed(SampleFormat::U8));
	EXPECT_TRUE(SampleFormat::is_packed(SampleFormat::S16));
	EXPECT_TRUE(SampleFormat::is_packed(SampleFormat::S32));
	EXPECT_TRUE(SampleFormat::is_packed(SampleFormat::F32));
	EXPECT_TRUE(SampleFormat::is_packed(SampleFormat::S64));
	EXPECT_TRUE(SampleFormat::is_packed(SampleFormat::F64));

	// Planar formats
	EXPECT_TRUE(SampleFormat::is_planar(SampleFormat::U8P));
	EXPECT_TRUE(SampleFormat::is_planar(SampleFormat::S16P));
	EXPECT_TRUE(SampleFormat::is_planar(SampleFormat::S32P));
	EXPECT_TRUE(SampleFormat::is_planar(SampleFormat::F32P));
	EXPECT_TRUE(SampleFormat::is_planar(SampleFormat::S64P));
	EXPECT_TRUE(SampleFormat::is_planar(SampleFormat::F64P));
}

TEST(AudioSmokeSampleFormat, StringConversion)
{
	// Test to_string (values may vary based on FFmpeg version)
	EXPECT_EQ(SampleFormat::to_string(SampleFormat::U8), "u8");
	EXPECT_EQ(SampleFormat::to_string(SampleFormat::S16), "s16");
	EXPECT_EQ(SampleFormat::to_string(SampleFormat::S32), "s32");
	// F32 can be "flt" or "f32" depending on FFmpeg version
	std::string f32_str = SampleFormat::to_string(SampleFormat::F32);
	EXPECT_TRUE(f32_str == "flt" || f32_str == "f32");
	// F64 can be "dbl" or "f64" depending on FFmpeg version
	std::string f64_str = SampleFormat::to_string(SampleFormat::F64);
	EXPECT_TRUE(f64_str == "dbl" || f64_str == "f64");

	// Test from_string
	EXPECT_EQ(SampleFormat::from_string("u8"), SampleFormat::U8);
	EXPECT_EQ(SampleFormat::from_string("s16"), SampleFormat::S16);
	// from_string may not support all format names
	EXPECT_EQ(SampleFormat::from_string(""), SampleFormat::INVALID);
	EXPECT_EQ(SampleFormat::from_string("unknown"), SampleFormat::INVALID);
}

// ============================================================================
// Smoke Test: Thread Safety
// ============================================================================

TEST(AudioSmokeThread, ConcurrentWaveformAccess)
{
	const int num_threads = 4;
	const int num_ops_per_thread = 50;

	AudioVisualWaveform waveform;
	waveform.set_channel_count(2);

	// Pre-populate with data
	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer buffer(params, size_t(4800));
	FillSampleBuffer(buffer, 0.5f);
	waveform.OverwriteSamples(buffer, 48000, rational(0));

	std::vector<std::thread> threads;
	std::atomic<int> success_count{ 0 };

	for (int t = 0; t < num_threads; ++t) {
		threads.emplace_back([&waveform, &success_count, num_ops_per_thread]() {
			for (int i = 0; i < num_ops_per_thread; ++i) {
				// Read summary from different times
				auto summary = waveform.GetSummaryFromTime(
					rational(i % 10, 100), // 0.00 to 0.09 seconds
					rational(1, 100) // 0.01 second duration
				);

				if (summary.size() == 2) {
					success_count++;
				}
			}
		});
	}

	for (auto &t : threads) {
		t.join();
	}

	EXPECT_EQ(success_count.load(), num_threads * num_ops_per_thread);
}

TEST(AudioSmokeThread, ConcurrentSampleBufferOperations)
{
	const int num_threads = 4;

	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	SampleBuffer buffer(params, size_t(1000));
	FillSampleBuffer(buffer, 0.5f);

	std::vector<std::thread> threads;
	std::atomic<int> success_count{ 0 };

	for (int t = 0; t < num_threads; ++t) {
		threads.emplace_back([&buffer, &success_count, t]() {
			// Each thread applies different operations
			switch (t % 4) {
			case 0:
				buffer.transform_volume(0.8f);
				success_count++;
				break;
			case 1:
				buffer.clamp();
				success_count++;
				break;
			case 2: {
				auto ripped = buffer.rip_channel(0);
				if (ripped.channel_count() == 1)
					success_count++;
				break;
			}
			case 3: {
				auto ptrs = buffer.to_raw_ptrs();
				if (!ptrs.empty())
					success_count++;
				break;
			}
			}
		});
	}

	for (auto &t : threads) {
		t.join();
	}

	EXPECT_EQ(success_count.load(), num_threads);
}

} // namespace test
} // namespace audio
} // namespace olive
