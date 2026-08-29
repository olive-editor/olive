#include <gtest/gtest.h>

#include "olive/core/render/samplebuffer.h"

using namespace olive::core;

static AudioParams MakeParams(int channels = 2, int sample_rate = 48000)
{
	AudioParams params(sample_rate, kChannelLayoutStereo, SampleFormat::F32P);
	return params;
}

TEST(CoreSampleBuffer, DefaultConstruction)
{
	SampleBuffer b;
	EXPECT_FALSE(b.is_allocated());
	EXPECT_EQ(b.channel_count(), 0);
	EXPECT_EQ(b.sample_count(), 0u);
}

TEST(CoreSampleBuffer, AllocateByLength)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, rational(1, 24));
	EXPECT_TRUE(b.is_allocated());
	EXPECT_EQ(b.channel_count(), 2);
	EXPECT_EQ(b.sample_count(), 2000u);
}

TEST(CoreSampleBuffer, AllocateBySampleCount)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 1024);
	EXPECT_TRUE(b.is_allocated());
	EXPECT_EQ(b.sample_count(), 1024u);
}

TEST(CoreSampleBuffer, AllocateInvalidParams)
{
	AudioParams params;
	SampleBuffer b(params, 100);
	EXPECT_FALSE(b.is_allocated());
}

TEST(CoreSampleBuffer, AllocateZeroSampleCount)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 0);
	EXPECT_FALSE(b.is_allocated());
}

TEST(CoreSampleBuffer, Destroy)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 100);
	EXPECT_TRUE(b.is_allocated());
	b.destroy();
	EXPECT_FALSE(b.is_allocated());
}

TEST(CoreSampleBuffer, SetAudioParamsBeforeAllocate)
{
	AudioParams params = MakeParams();
	SampleBuffer b;
	b.set_audio_params(params);
	b.set_sample_count(100);
	b.allocate();
	EXPECT_TRUE(b.is_allocated());
}

TEST(CoreSampleBuffer, SetParamsOnAllocatedIsIgnored)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 100);
	AudioParams other;
	b.set_audio_params(other);
	EXPECT_EQ(b.audio_params().sample_rate(), params.sample_rate());
}

TEST(CoreSampleBuffer, SetSampleCountOnAllocatedIsIgnored)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 100);
	b.set_sample_count(200);
	EXPECT_EQ(b.sample_count(), 100u);
}

TEST(CoreSampleBuffer, DataAccess)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 4);
	b.data(0)[0] = 0.1f;
	b.data(0)[1] = 0.2f;
	EXPECT_FLOAT_EQ(b.data(0)[0], 0.1f);
	EXPECT_FLOAT_EQ(b.data(0)[1], 0.2f);
}

TEST(CoreSampleBuffer, ToRawPtrs)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 4);
	std::vector<float *> ptrs = b.to_raw_ptrs();
	EXPECT_EQ(ptrs.size(), 2u);
	EXPECT_NE(ptrs[0], nullptr);
	EXPECT_NE(ptrs[1], nullptr);
}

TEST(CoreSampleBuffer, RipChannel)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 4);
	b.data(0)[0] = 0.5f;
	b.data(1)[0] = 0.7f;

	SampleBuffer mono = b.rip_channel(1);
	EXPECT_EQ(mono.channel_count(), 1);
	EXPECT_FLOAT_EQ(mono.data(0)[0], 0.7f);
}

TEST(CoreSampleBuffer, RipChannelVector)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 4);
	b.data(0)[0] = 0.3f;

	std::vector<float> v = b.rip_channel_vector(0);
	EXPECT_EQ(v.size(), 4u);
	EXPECT_FLOAT_EQ(v[0], 0.3f);
}

TEST(CoreSampleBuffer, TransformVolume)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 4);
	b.data(0)[0] = 0.5f;
	b.transform_volume(2.0f);
	EXPECT_FLOAT_EQ(b.data(0)[0], 1.0f);
}

TEST(CoreSampleBuffer, TransformVolumeForChannel)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 4);
	b.data(0)[0] = 0.5f;
	b.data(1)[0] = 0.5f;
	b.transform_volume_for_channel(1, 3.0f);
	EXPECT_FLOAT_EQ(b.data(0)[0], 0.5f);
	EXPECT_FLOAT_EQ(b.data(1)[0], 1.5f);
}

TEST(CoreSampleBuffer, TransformVolumeStatic)
{
	AudioParams params = MakeParams();
	SampleBuffer in(params, 4);
	SampleBuffer out(params, 4);
	in.data(0)[0] = 0.5f;
	SampleBuffer::transform_volume(2.0f, &in, &out);
	EXPECT_FLOAT_EQ(out.data(0)[0], 1.0f);
}

TEST(CoreSampleBuffer, TransformVolumeForSample)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 4);
	b.data(0)[0] = 0.5f;
	b.data(1)[0] = 0.5f;
	b.transform_volume_for_sample(0, 2.0f);
	EXPECT_FLOAT_EQ(b.data(0)[0], 1.0f);
	EXPECT_FLOAT_EQ(b.data(1)[0], 1.0f);
}

TEST(CoreSampleBuffer, Clamp)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 4);
	b.data(0)[0] = 2.0f;
	b.data(0)[1] = -2.0f;
	b.clamp();
	EXPECT_FLOAT_EQ(b.data(0)[0], 1.0f);
	EXPECT_FLOAT_EQ(b.data(0)[1], -1.0f);
}

TEST(CoreSampleBuffer, Silence)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 4);
	b.data(0)[0] = 1.0f;
	b.silence();
	EXPECT_FLOAT_EQ(b.data(0)[0], 0.0f);
}

TEST(CoreSampleBuffer, SilenceRange)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 4);
	b.data(0)[0] = 1.0f;
	b.data(0)[1] = 1.0f;
	b.data(0)[2] = 1.0f;
	b.data(0)[3] = 1.0f;
	b.silence(1, 3);
	EXPECT_FLOAT_EQ(b.data(0)[0], 1.0f);
	EXPECT_FLOAT_EQ(b.data(0)[1], 0.0f);
	EXPECT_FLOAT_EQ(b.data(0)[2], 0.0f);
	EXPECT_FLOAT_EQ(b.data(0)[3], 1.0f);
}

TEST(CoreSampleBuffer, Set)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 4);
	float data[2] = { 0.3f, 0.4f };
	b.set(0, data, 1, 2);
	EXPECT_FLOAT_EQ(b.data(0)[1], 0.3f);
	EXPECT_FLOAT_EQ(b.data(0)[2], 0.4f);
}

TEST(CoreSampleBuffer, FastSet)
{
	AudioParams params = MakeParams();
	SampleBuffer src(params, 4);
	SampleBuffer dst(params, 4);
	src.data(1)[0] = 0.9f;
	dst.fast_set(src, 0, 1);
	EXPECT_FLOAT_EQ(dst.data(0)[0], 0.9f);
}

TEST(CoreSampleBuffer, Reverse)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 4);
	b.data(0)[0] = 0.1f;
	b.data(0)[1] = 0.2f;
	b.data(0)[2] = 0.3f;
	b.data(0)[3] = 0.4f;
	b.reverse();
	EXPECT_FLOAT_EQ(b.data(0)[0], 0.4f);
	EXPECT_FLOAT_EQ(b.data(0)[3], 0.1f);
}

TEST(CoreSampleBuffer, Speed)
{
	AudioParams params = MakeParams();
	SampleBuffer b(params, 4);
	b.data(0)[0] = 0.1f;
	b.data(0)[1] = 0.2f;
	b.data(0)[2] = 0.3f;
	b.data(0)[3] = 0.4f;
	b.speed(2.0);
	EXPECT_EQ(b.sample_count(), 2u);
	EXPECT_FLOAT_EQ(b.data(0)[0], 0.1f);
	EXPECT_FLOAT_EQ(b.data(0)[1], 0.3f);
}

TEST(CoreSampleBuffer, UnallocatedOperationsNoCrash)
{
	SampleBuffer b;
	b.reverse();
	b.speed(2.0);
	b.silence();
	b.set(0, nullptr, 0, 0);
	b.destroy();
}
