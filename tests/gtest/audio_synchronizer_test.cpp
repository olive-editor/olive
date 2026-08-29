#include <gtest/gtest.h>

#include "audio/audiosynchronizer.h"

TEST(AudioSynchronizer, PlacesCandidateBySourceStartTime)
{
	olive::AudioSynchronizer::SourceClip reference;
	reference.source_start_time = olive::core::rational(100);
	reference.has_source_start_time = true;

	olive::AudioSynchronizer::SourceClip candidate;
	candidate.source_start_time = olive::core::rational(112);
	candidate.has_source_start_time = true;

	const olive::AudioSynchronizer::Placement placement =
		olive::AudioSynchronizer::PlaceBySourceTime(reference, candidate,
													olive::core::rational(10));

	ASSERT_TRUE(placement.valid);
	EXPECT_EQ(placement.timeline_in, olive::core::rational(22));
}

TEST(AudioSynchronizer, AccountsForMediaInWhenPlacingBySourceTime)
{
	olive::AudioSynchronizer::SourceClip reference;
	reference.source_start_time = olive::core::rational(100);
	reference.media_in = olive::core::rational(2);
	reference.has_source_start_time = true;

	olive::AudioSynchronizer::SourceClip candidate;
	candidate.source_start_time = olive::core::rational(100);
	candidate.media_in = olive::core::rational(5);
	candidate.has_source_start_time = true;

	const olive::AudioSynchronizer::Placement placement =
		olive::AudioSynchronizer::PlaceBySourceTime(reference, candidate,
													olive::core::rational(20));

	ASSERT_TRUE(placement.valid);
	EXPECT_EQ(placement.timeline_in, olive::core::rational(23));
}

TEST(AudioSynchronizer, RejectsMissingSourceStartTime)
{
	olive::AudioSynchronizer::SourceClip reference;
	reference.source_start_time = olive::core::rational(100);
	reference.has_source_start_time = true;

	olive::AudioSynchronizer::SourceClip candidate;

	const olive::AudioSynchronizer::Placement placement =
		olive::AudioSynchronizer::PlaceBySourceTime(reference, candidate,
													olive::core::rational(10));

	EXPECT_FALSE(placement.valid);
}

TEST(AudioSynchronizer, PlacesCandidateByWaveformOffset)
{
	const olive::AudioSynchronizer::Placement placement =
		olive::AudioSynchronizer::PlaceByWaveformOffset(
			olive::core::rational(10), 24000, 48000);

	ASSERT_TRUE(placement.valid);
	EXPECT_EQ(placement.timeline_in, olive::core::rational(21, 2));
}

TEST(AudioSynchronizer, SupportsCandidateLeadByWaveformOffset)
{
	const olive::AudioSynchronizer::Placement placement =
		olive::AudioSynchronizer::PlaceByWaveformOffset(
			olive::core::rational(10), -48000, 48000);

	ASSERT_TRUE(placement.valid);
	EXPECT_EQ(placement.timeline_in, olive::core::rational(9));
}
