#include <gtest/gtest.h>

#include "audio/audiowaveformsync.h"
#include "olive/core/render/audioparams.h"
#include "olive/core/render/samplebuffer.h"
#include "olive/core/render/sampleformat.h"

namespace
{

olive::core::AudioParams MakeMonoParams()
{
	return olive::core::AudioParams(48000, olive::core::kChannelLayoutMono,
									olive::core::SampleFormat::F32P);
}

olive::core::SampleBuffer MakeBuffer(const QVector<float> &values)
{
	olive::core::SampleBuffer samples(MakeMonoParams(),
									  static_cast<size_t>(values.size()));
	float *data = samples.data(0);
	for (int i = 0; i < values.size(); i++) {
		data[i] = values.at(i);
	}
	return samples;
}

}

TEST(AudioWaveformSync, ExtractsRmsEnvelope)
{
	olive::core::SampleBuffer samples =
		MakeBuffer({ 1.0f, -1.0f, 0.5f, -0.5f, 0.0f, 0.0f });

	const QVector<double> envelope =
		olive::AudioWaveformSync::ExtractRmsEnvelope(samples, 2);

	ASSERT_EQ(envelope.size(), 3);
	EXPECT_NEAR(envelope.at(0), 1.0, 0.0001);
	EXPECT_NEAR(envelope.at(1), 0.5, 0.0001);
	EXPECT_NEAR(envelope.at(2), 0.0, 0.0001);
}

TEST(AudioWaveformSync, EstimatesCandidateLag)
{
	const QVector<float> reference_values = { 0.0f, 0.0f, 0.8f, 0.8f, 0.1f,
											  0.1f, 0.6f, 0.6f, 0.0f, 0.0f };
	const QVector<float> candidate_values = { 0.0f, 0.0f, 0.0f, 0.0f,
											  0.8f, 0.8f, 0.1f, 0.1f,
											  0.6f, 0.6f, 0.0f, 0.0f };

	const olive::AudioWaveformSync::OffsetResult result =
		olive::AudioWaveformSync::EstimateOffset(
			MakeBuffer(reference_values), MakeBuffer(candidate_values), 2, 8);

	ASSERT_TRUE(result.valid);
	EXPECT_EQ(result.offset_samples, 2);
	EXPECT_GT(result.confidence, 0.99);
}

TEST(AudioWaveformSync, EstimatesCandidateLead)
{
	const QVector<float> reference_values = { 0.0f, 0.0f, 0.0f, 0.0f,
											  0.9f, 0.9f, 0.3f, 0.3f,
											  0.7f, 0.7f, 0.0f, 0.0f };
	const QVector<float> candidate_values = { 0.9f, 0.9f, 0.3f, 0.3f,
											  0.7f, 0.7f, 0.0f, 0.0f };

	const olive::AudioWaveformSync::OffsetResult result =
		olive::AudioWaveformSync::EstimateOffset(
			MakeBuffer(reference_values), MakeBuffer(candidate_values), 2, 4);

	ASSERT_TRUE(result.valid);
	EXPECT_EQ(result.offset_samples, -4);
	EXPECT_GT(result.confidence, 0.99);
}

TEST(AudioWaveformSync, RejectsSilence)
{
	olive::core::SampleBuffer reference(MakeMonoParams(), size_t(16));
	olive::core::SampleBuffer candidate(MakeMonoParams(), size_t(16));
	reference.silence();
	candidate.silence();

	const olive::AudioWaveformSync::OffsetResult result =
		olive::AudioWaveformSync::EstimateOffset(reference, candidate, 4, 16);

	EXPECT_FALSE(result.valid);
}

TEST(AudioWaveformSync, MaskedEstimationIgnoresInvalidWindows)
{
	const QVector<double> reference = { 0.5, 0.5, 0.5, 0.5, 0.9, 0.1,
										0.7, 0.2, 0.8, 0.3, 0.6, 0.4 };

	// Candidate is the reference delayed by 3 windows, but windows 3..7 were
	// never cached (zeroed out) and are flagged invalid
	QVector<double> candidate(12, 0.0);
	QVector<bool> candidate_valid(12, true);
	for (int i = 0; i + 3 < candidate.size(); i++) {
		candidate[i + 3] = reference.at(i);
	}
	for (int i = 3; i <= 7; i++) {
		candidate[i] = 0.0;
		candidate_valid[i] = false;
	}

	const olive::AudioWaveformSync::OffsetResult unmasked =
		olive::AudioWaveformSync::EstimateEnvelopeOffset(reference, candidate, 1,
														 8);
	const olive::AudioWaveformSync::OffsetResult masked =
		olive::AudioWaveformSync::EstimateEnvelopeOffset(
			reference, candidate, QVector<bool>(), candidate_valid, 1, 8);

	ASSERT_TRUE(masked.valid);
	EXPECT_EQ(masked.offset_samples, 3);
	EXPECT_GT(masked.confidence, 0.99);

	// Ignoring the uncached placeholder windows must not make the estimate
	// worse than treating them as silence
	if (unmasked.valid) {
		EXPECT_GE(masked.confidence, unmasked.confidence);
	}
}

TEST(AudioWaveformSync, EstimatesStretchAndOffset)
{
	// 20-window reference pattern
	const QVector<double> reference = { 0.1, 0.9, 0.2, 0.8, 0.3,
										0.7, 0.4, 0.6, 0.5, 1.0,
										0.15, 0.85, 0.25, 0.75, 0.35,
										0.65, 0.45, 0.55, 0.95, 0.05 };

	// Candidate runs at half speed (each window duplicated) and is delayed by
	// 6 candidate windows: candidate[j] = reference[(j-6)/2]
	QVector<double> candidate(6 + 2 * reference.size(), 0.0);
	for (int i = 0; i < reference.size(); i++) {
		candidate[6 + 2 * i] = reference.at(i);
		candidate[6 + 2 * i + 1] = reference.at(i);
	}

	const olive::AudioWaveformSync::StretchOffsetResult result =
		olive::AudioWaveformSync::EstimateStretchAndOffset(
			reference, candidate, QVector<bool>(), QVector<bool>(), 1, 12,
			0.8, 2.5, 0.005);

	ASSERT_TRUE(result.valid);
	EXPECT_NEAR(result.rate, 2.0, 0.01);
	// After resampling at 2x, the candidate lags the reference by 3 windows
	EXPECT_EQ(result.offset_samples, 3);
	EXPECT_GT(result.confidence, 0.95);
}

TEST(AudioWaveformSync, StretchEstimationRejectsSilence)
{
	const QVector<double> silence(16, 0.0);

	const olive::AudioWaveformSync::StretchOffsetResult result =
		olive::AudioWaveformSync::EstimateStretchAndOffset(
			silence, silence, QVector<bool>(), QVector<bool>(), 1, 8, 0.5,
			2.0, 0.1);

	EXPECT_FALSE(result.valid);
}

TEST(AudioWaveformSync, StretchEstimationRejectsInvalidParameters)
{
	const QVector<double> envelope = { 0.5, 0.6, 0.7, 0.8 };

	EXPECT_FALSE(olive::AudioWaveformSync::EstimateStretchAndOffset(
					 envelope, envelope, QVector<bool>(), QVector<bool>(), 1,
					 8, 0.0, 2.0, 0.1)
					 .valid);
	EXPECT_FALSE(olive::AudioWaveformSync::EstimateStretchAndOffset(
					 envelope, envelope, QVector<bool>(), QVector<bool>(), 1,
					 8, 2.0, 0.5, 0.1)
					 .valid);
	EXPECT_FALSE(olive::AudioWaveformSync::EstimateStretchAndOffset(
					 envelope, envelope, QVector<bool>(), QVector<bool>(), 1,
					 8, 0.5, 2.0, 0.0)
					 .valid);
}
