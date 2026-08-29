#include <gtest/gtest.h>

#include <QVector>

#include "node/block/clip/clip.h"
#include "node/color/colormanager/colormanager.h"
#include "node/math/math/math.h"
#include "node/output/track/track.h"
#include "node/output/track/tracklist.h"
#include "node/output/viewer/viewer.h"
#include "node/project.h"
#include "node/project/sequence/sequence.h"

namespace
{

olive::Sequence *CreateSequence(olive::Project *project)
{
	auto *sequence = new olive::Sequence();
	sequence->setParent(project);
	return sequence;
}

olive::Track *CreateTrack(olive::Project *project)
{
	auto *track = new olive::Track();
	track->setParent(project);
	return track;
}

olive::ClipBlock *CreateClip(olive::Project *project,
							 const olive::core::rational &length)
{
	auto *clip = new olive::ClipBlock();
	clip->setParent(project);
	clip->set_length_and_media_out(length);
	return clip;
}

// Mirrors what TimelineAddTrackCommand::redo() does to wire a track into a
// sequence: grow the track input array, then connect the edge
void AppendTrackToList(olive::TrackList *list, olive::Track *track)
{
	list->ArrayAppend();
	olive::Node::ConnectEdge(track,
							 list->track_input(list->ArraySize() - 1));
}

} // namespace

TEST(Sequence, DefaultState)
{
	olive::Sequence sequence;

	EXPECT_EQ(sequence.Name(), QStringLiteral("Sequence"));
	EXPECT_EQ(sequence.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.sequence"));
	EXPECT_TRUE(sequence.Category().contains(olive::Node::kCategoryProject));

	// Track input ids are generated from kTrackInputFormat
	EXPECT_EQ(olive::Sequence::kTrackInputFormat.arg(0),
			  QStringLiteral("track_in_0"));
	EXPECT_EQ(olive::Sequence::kTrackInputFormat.arg(1),
			  QStringLiteral("track_in_1"));
	EXPECT_EQ(olive::Sequence::kTrackInputFormat.arg(2),
			  QStringLiteral("track_in_2"));

	// One track list per track type, all empty
	olive::TrackList *video = sequence.track_list(olive::Track::kVideo);
	olive::TrackList *audio = sequence.track_list(olive::Track::kAudio);
	olive::TrackList *subtitle = sequence.track_list(olive::Track::kSubtitle);
	ASSERT_NE(video, nullptr);
	ASSERT_NE(audio, nullptr);
	ASSERT_NE(subtitle, nullptr);

	EXPECT_EQ(video->type(), olive::Track::kVideo);
	EXPECT_EQ(audio->type(), olive::Track::kAudio);
	EXPECT_EQ(subtitle->type(), olive::Track::kSubtitle);

	EXPECT_EQ(video->track_input(), QStringLiteral("track_in_0"));
	EXPECT_EQ(audio->track_input(), QStringLiteral("track_in_1"));
	EXPECT_EQ(subtitle->track_input(), QStringLiteral("track_in_2"));

	EXPECT_EQ(video->parent(), &sequence);
	EXPECT_EQ(audio->parent(), &sequence);
	EXPECT_EQ(subtitle->parent(), &sequence);

	EXPECT_EQ(video->GetTrackCount(), 0);
	EXPECT_EQ(video->GetTotalLength(), olive::core::rational(0));
	EXPECT_EQ(video->ArraySize(), 0);
	EXPECT_EQ(video->GetTrackAt(0), nullptr);
	EXPECT_EQ(video->GetTrackAt(-1), nullptr);

	EXPECT_TRUE(sequence.GetTracks().isEmpty());
	EXPECT_TRUE(sequence.GetUnlockedTracks().isEmpty());
	EXPECT_EQ(sequence.GetTrackFromReference(
				  olive::Track::Reference(olive::Track::kVideo, 0)),
			  nullptr);

	// Invalid and out-of-range reference types return null instead of crashing
	EXPECT_EQ(sequence.GetTrackFromReference(
				  olive::Track::Reference(olive::Track::kNone, 0)),
			  nullptr);
	EXPECT_EQ(sequence.GetTrackFromReference(
				  olive::Track::Reference(olive::Track::kCount, 0)),
			  nullptr);

	// Length verification over empty track lists keeps everything at zero
	sequence.VerifyLength();
	EXPECT_EQ(sequence.GetLength(), olive::core::rational(0));
	EXPECT_EQ(sequence.GetVideoLength(), olive::core::rational(0));
	EXPECT_EQ(sequence.GetAudioLength(), olive::core::rational(0));
	EXPECT_EQ(sequence.GetPlayhead(), olive::core::rational(0));
}

TEST(Sequence, RetranslateSetsTrackInputNames)
{
	olive::Sequence sequence;

	sequence.Retranslate();

	EXPECT_EQ(sequence.GetInputName(olive::Sequence::kTrackInputFormat.arg(
					  olive::Track::kVideo)),
			  QStringLiteral("Video Tracks"));
	EXPECT_EQ(sequence.GetInputName(olive::Sequence::kTrackInputFormat.arg(
					  olive::Track::kAudio)),
			  QStringLiteral("Audio Tracks"));
	EXPECT_EQ(sequence.GetInputName(olive::Sequence::kTrackInputFormat.arg(
					  olive::Track::kSubtitle)),
			  QStringLiteral("Subtitle Tracks"));
}

TEST(Sequence, AddDefaultNodesCreatesVideoAndAudioTracks)
{
	olive::ColorManager::SetUpDefaultConfig();
	QVector<olive::Track *> added;
	olive::Project project;
	project.Initialize();
	olive::Sequence *sequence = CreateSequence(&project);

	QObject::connect(sequence, &olive::Sequence::TrackAdded,
					 [&added](olive::Track *t) { added.append(t); });

	sequence->add_default_nodes();

	olive::TrackList *video_list = sequence->track_list(olive::Track::kVideo);
	olive::TrackList *audio_list = sequence->track_list(olive::Track::kAudio);
	olive::TrackList *subtitle_list =
		sequence->track_list(olive::Track::kSubtitle);

	// One video and one audio track, no subtitle tracks
	ASSERT_EQ(video_list->GetTrackCount(), 1);
	ASSERT_EQ(audio_list->GetTrackCount(), 1);
	EXPECT_EQ(subtitle_list->GetTrackCount(), 0);

	olive::Track *video_track = video_list->GetTrackAt(0);
	olive::Track *audio_track = audio_list->GetTrackAt(0);
	ASSERT_NE(video_track, nullptr);
	ASSERT_NE(audio_track, nullptr);

	EXPECT_EQ(video_track->type(), olive::Track::kVideo);
	EXPECT_EQ(audio_track->type(), olive::Track::kAudio);
	EXPECT_EQ(video_track->sequence(), sequence);
	EXPECT_EQ(audio_track->sequence(), sequence);
	EXPECT_EQ(video_track->Index(), 0);
	EXPECT_EQ(audio_track->Index(), 0);

	// Both tracks were reparented into the sequence's project
	EXPECT_EQ(video_track->parent(), &project);
	EXPECT_EQ(audio_track->parent(), &project);
	EXPECT_EQ(video_list->GetParentGraph(), &project);

	// The sequence forwards TrackAdded from its track lists
	ASSERT_EQ(added.size(), 2);
	EXPECT_TRUE(added.contains(video_track));
	EXPECT_TRUE(added.contains(audio_track));

	// The flattened track cache contains both tracks
	EXPECT_EQ(sequence->GetTracks().size(), 2);
	EXPECT_TRUE(sequence->GetTracks().contains(video_track));
	EXPECT_TRUE(sequence->GetTracks().contains(audio_track));
	EXPECT_EQ(sequence->GetUnlockedTracks(), sequence->GetTracks());

	// Track lookup by reference
	EXPECT_EQ(sequence->GetTrackFromReference(
				  olive::Track::Reference(olive::Track::kVideo, 0)),
			  video_track);
	EXPECT_EQ(sequence->GetTrackFromReference(
				  olive::Track::Reference(olive::Track::kAudio, 0)),
			  audio_track);
	EXPECT_EQ(sequence->GetTrackFromReference(
				  olive::Track::Reference(olive::Track::kSubtitle, 0)),
			  nullptr);

	// The default tracks are wired straight into the viewer outputs
	EXPECT_TRUE(sequence->IsInputConnected(olive::ViewerOutput::kTextureInput));
	EXPECT_TRUE(sequence->IsInputConnected(olive::ViewerOutput::kSamplesInput));
	EXPECT_EQ(sequence->GetConnectedTextureOutput(), video_track);
	EXPECT_EQ(sequence->GetConnectedSampleOutput(), audio_track);
}

TEST(Sequence, TrackConnectEmitsSignalsAndSetsTrackState)
{
	olive::ColorManager::SetUpDefaultConfig();
	int list_added = 0;
	int list_changed = 0;
	int sequence_added = 0;
	olive::Project project;
	project.Initialize();
	olive::Sequence *sequence = CreateSequence(&project);
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);
	olive::Track *track = CreateTrack(&project);

	olive::Track *list_last_added = nullptr;
	QObject::connect(list, &olive::TrackList::TrackAdded,
					 [&list_added, &list_last_added](olive::Track *t) {
						 ++list_added;
						 list_last_added = t;
					 });
	QObject::connect(list, &olive::TrackList::TrackListChanged,
					 [&list_changed]() { ++list_changed; });
	olive::Track *sequence_last_added = nullptr;
	QObject::connect(sequence, &olive::Sequence::TrackAdded,
					 [&sequence_added, &sequence_last_added](olive::Track *t) {
						 ++sequence_added;
						 sequence_last_added = t;
					 });

	list->ArrayAppend();
	EXPECT_EQ(list->ArraySize(), 1);
	EXPECT_EQ(list->GetTrackCount(), 0);

	olive::Node::ConnectEdge(track, list->track_input(0));

	EXPECT_EQ(list_added, 1);
	EXPECT_EQ(list_last_added, track);
	EXPECT_EQ(list_changed, 1);
	EXPECT_EQ(sequence_added, 1);
	EXPECT_EQ(sequence_last_added, track);

	// The track adopts the list's type, sequence and cache index
	EXPECT_EQ(track->type(), olive::Track::kVideo);
	EXPECT_EQ(track->sequence(), sequence);
	EXPECT_EQ(track->Index(), 0);

	EXPECT_EQ(list->GetTrackCount(), 1);
	EXPECT_EQ(list->GetTrackAt(0), track);
	EXPECT_EQ(list->GetArrayIndexFromCacheIndex(0), 0);
	EXPECT_EQ(list->GetCacheIndexFromArrayIndex(0), 0);
	EXPECT_EQ(list->GetParentGraph(), &project);

	// TrackList::track_input() builds a NodeInput pointing at the sequence
	const olive::NodeInput input = list->track_input(0);
	EXPECT_EQ(input, olive::NodeInput(
						 sequence, olive::Sequence::kTrackInputFormat.arg(
										   olive::Track::kVideo),
						 0));

	// The sequence-level cache tracks the new track
	EXPECT_EQ(sequence->GetTracks(), QVector<olive::Track *>({ track }));
	EXPECT_EQ(sequence->GetTrackFromReference(
				  olive::Track::Reference(olive::Track::kVideo, 0)),
			  track);
}

TEST(Sequence, TrackDisconnectResetsTrackState)
{
	olive::ColorManager::SetUpDefaultConfig();
	int list_removed = 0;
	int sequence_removed = 0;
	olive::Project project;
	project.Initialize();
	olive::Sequence *sequence = CreateSequence(&project);
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);

	olive::Track *first = CreateTrack(&project);
	olive::Track *second = CreateTrack(&project);
	AppendTrackToList(list, first);
	AppendTrackToList(list, second);
	ASSERT_EQ(list->GetTrackCount(), 2);

	olive::Track *list_last_removed = nullptr;
	QObject::connect(list, &olive::TrackList::TrackRemoved,
					 [&list_removed, &list_last_removed](olive::Track *t) {
						 ++list_removed;
						 list_last_removed = t;
					 });
	QObject::connect(sequence, &olive::Sequence::TrackRemoved,
					 [&sequence_removed](olive::Track *) {
						 ++sequence_removed;
					 });

	// While connected, track height changes are forwarded by the list
	int height_changed = 0;
	QObject::connect(list, &olive::TrackList::TrackHeightChanged,
					 [&height_changed](olive::Track *, int) {
						 ++height_changed;
					 });
	first->SetTrackHeight(first->GetTrackHeight() + 1.0);
	EXPECT_EQ(height_changed, 1);

	olive::Node::DisconnectEdge(first, list->track_input(0));

	EXPECT_EQ(list_removed, 1);
	EXPECT_EQ(list_last_removed, first);
	EXPECT_EQ(sequence_removed, 1);

	// The removed track is fully detached from the sequence
	EXPECT_EQ(first->sequence(), nullptr);
	EXPECT_EQ(first->type(), olive::Track::kNone);
	EXPECT_EQ(first->Index(), -1);

	// Subsequent tracks shift down in the cache and get re-indexed
	EXPECT_EQ(list->GetTrackCount(), 1);
	EXPECT_EQ(list->GetTrackAt(0), second);
	EXPECT_EQ(second->Index(), 0);

	// The array element itself stays; only the cache mapping moves
	EXPECT_EQ(list->ArraySize(), 2);
	EXPECT_EQ(list->GetCacheIndexFromArrayIndex(0), -1);
	EXPECT_EQ(list->GetCacheIndexFromArrayIndex(1), 0);
	EXPECT_EQ(list->GetArrayIndexFromCacheIndex(0), 1);

	EXPECT_EQ(sequence->GetTracks(), QVector<olive::Track *>({ second }));
	EXPECT_EQ(sequence->GetTrackFromReference(
				  olive::Track::Reference(olive::Track::kVideo, 0)),
			  second);
	EXPECT_EQ(sequence->GetTrackFromReference(
				  olive::Track::Reference(olive::Track::kVideo, 1)),
			  nullptr);

	// Height changes on the removed track must no longer be forwarded
	first->SetTrackHeight(first->GetTrackHeight() + 1.0);
	EXPECT_EQ(height_changed, 1);
}

TEST(TrackList, CacheOrderFollowsArrayIndex)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Sequence *sequence = CreateSequence(&project);
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);

	olive::Track *first = CreateTrack(&project);
	olive::Track *second = CreateTrack(&project);

	list->ArrayAppend();
	list->ArrayAppend();

	// Connect the higher array element first; it takes cache index 0 for now
	olive::Node::ConnectEdge(second, list->track_input(1));
	EXPECT_EQ(list->GetTrackCount(), 1);
	EXPECT_EQ(list->GetTrackAt(0), second);
	EXPECT_EQ(second->Index(), 0);
	EXPECT_EQ(list->GetCacheIndexFromArrayIndex(0), -1);
	EXPECT_EQ(list->GetCacheIndexFromArrayIndex(1), 0);

	// Connecting element 0 inserts ahead of it in the cache
	olive::Node::ConnectEdge(first, list->track_input(0));
	EXPECT_EQ(list->GetTrackCount(), 2);
	EXPECT_EQ(list->GetTrackAt(0), first);
	EXPECT_EQ(list->GetTrackAt(1), second);
	EXPECT_EQ(first->Index(), 0);
	EXPECT_EQ(second->Index(), 1);
	EXPECT_EQ(list->GetCacheIndexFromArrayIndex(0), 0);
	EXPECT_EQ(list->GetCacheIndexFromArrayIndex(1), 1);

	// Disconnecting element 0 re-indexes the remainder of the cache
	olive::Node::DisconnectEdge(first, list->track_input(0));
	EXPECT_EQ(list->GetTrackCount(), 1);
	EXPECT_EQ(list->GetTrackAt(0), second);
	EXPECT_EQ(second->Index(), 0);
}

TEST(TrackList, ArrayAppendAndRemoveLast)
{
	olive::Sequence sequence;
	olive::TrackList *list = sequence.track_list(olive::Track::kAudio);

	EXPECT_EQ(list->ArraySize(), 0);

	list->ArrayAppend();
	EXPECT_EQ(list->ArraySize(), 1);
	EXPECT_EQ(list->GetTrackCount(), 0);

	list->ArrayAppend();
	EXPECT_EQ(list->ArraySize(), 2);
	EXPECT_EQ(list->GetTrackCount(), 0);

	list->ArrayRemoveLast();
	EXPECT_EQ(list->ArraySize(), 1);

	list->ArrayRemoveLast();
	EXPECT_EQ(list->ArraySize(), 0);
}

TEST(TrackList, NonTrackAndArrayWideConnectionsAreIgnored)
{
	olive::Sequence sequence;
	olive::TrackList *list = sequence.track_list(olive::Track::kVideo);

	olive::MathNode math;
	olive::Track track;

	int changed = 0;
	QObject::connect(list, &olive::TrackList::TrackListChanged,
					 [&changed]() { ++changed; });

	// Nodes that are not Tracks never enter the cache
	list->TrackConnected(&math, 0);
	EXPECT_EQ(list->GetTrackCount(), 0);
	EXPECT_EQ(changed, 0);

	list->TrackDisconnected(&math, 0);
	EXPECT_EQ(list->GetTrackCount(), 0);
	EXPECT_EQ(changed, 0);

	// Element -1 means the whole array was replaced; the cache is left alone
	list->TrackConnected(&track, -1);
	EXPECT_EQ(list->GetTrackCount(), 0);
	EXPECT_EQ(changed, 0);
	EXPECT_EQ(track.sequence(), nullptr);

	list->TrackDisconnected(&track, -1);
	EXPECT_EQ(list->GetTrackCount(), 0);
	EXPECT_EQ(changed, 0);
}

TEST(Sequence, LengthFlowsFromTracksToLengthCache)
{
	olive::ColorManager::SetUpDefaultConfig();
	QVector<olive::core::rational> lengths;
	olive::Project project;
	project.Initialize();
	olive::Sequence *sequence = CreateSequence(&project);

	QObject::connect(sequence, &olive::ViewerOutput::LengthChanged,
					 [&lengths](const olive::core::rational &r) {
						 lengths.append(r);
					 });

	// A video track with content drives the video length and total length
	olive::Track *video_track = CreateTrack(&project);
	video_track->AppendBlock(CreateClip(&project, olive::core::rational(5)));
	AppendTrackToList(sequence->track_list(olive::Track::kVideo), video_track);

	EXPECT_EQ(sequence->GetVideoLength(), olive::core::rational(5));
	EXPECT_EQ(sequence->GetAudioLength(), olive::core::rational(0));
	EXPECT_EQ(sequence->GetLength(), olive::core::rational(5));

	// A longer audio track takes over the total length
	olive::Track *audio_track = CreateTrack(&project);
	audio_track->AppendBlock(CreateClip(&project, olive::core::rational(7)));
	AppendTrackToList(sequence->track_list(olive::Track::kAudio), audio_track);

	EXPECT_EQ(sequence->GetVideoLength(), olive::core::rational(5));
	EXPECT_EQ(sequence->GetAudioLength(), olive::core::rational(7));
	EXPECT_EQ(sequence->GetLength(), olive::core::rational(7));

	// Extending a connected track ripples through to the sequence length
	video_track->AppendBlock(CreateClip(&project, olive::core::rational(5)));

	EXPECT_EQ(sequence->GetVideoLength(), olive::core::rational(10));
	EXPECT_EQ(sequence->GetAudioLength(), olive::core::rational(7));
	EXPECT_EQ(sequence->GetLength(), olive::core::rational(10));

	// LengthChanged only fires when the total length actually changes
	EXPECT_EQ(lengths,
			  QVector<olive::core::rational>({ olive::core::rational(5),
											   olive::core::rational(7),
											   olive::core::rational(10) }));
}

TEST(Sequence, SubtitleTrackLengthContributesToTotalLength)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Sequence *sequence = CreateSequence(&project);

	olive::Track *subtitle_track = CreateTrack(&project);
	subtitle_track->AppendBlock(CreateClip(&project, olive::core::rational(3)));
	AppendTrackToList(sequence->track_list(olive::Track::kSubtitle),
					  subtitle_track);

	EXPECT_EQ(subtitle_track->type(), olive::Track::kSubtitle);
	EXPECT_EQ(sequence->GetVideoLength(), olive::core::rational(0));
	EXPECT_EQ(sequence->GetAudioLength(), olive::core::rational(0));
	EXPECT_EQ(sequence->GetLength(), olive::core::rational(3));
}

TEST(Sequence, GetUnlockedTracksOmitsLocked)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();
	olive::Sequence *sequence = CreateSequence(&project);

	olive::Track *video_a = CreateTrack(&project);
	olive::Track *video_b = CreateTrack(&project);
	olive::Track *audio = CreateTrack(&project);
	AppendTrackToList(sequence->track_list(olive::Track::kVideo), video_a);
	AppendTrackToList(sequence->track_list(olive::Track::kVideo), video_b);
	AppendTrackToList(sequence->track_list(olive::Track::kAudio), audio);

	// The flattened cache is ordered by track type (video, then audio)
	EXPECT_EQ(sequence->GetTracks(),
			  QVector<olive::Track *>({ video_a, video_b, audio }));
	EXPECT_EQ(sequence->GetUnlockedTracks(), sequence->GetTracks());

	video_b->SetLocked(true);
	EXPECT_EQ(sequence->GetUnlockedTracks(),
			  QVector<olive::Track *>({ video_a, audio }));

	video_b->SetLocked(false);
	EXPECT_EQ(sequence->GetUnlockedTracks(), sequence->GetTracks());
}

TEST(Sequence, SubtitleInvalidateEmitsSubtitlesChanged)
{
	olive::ColorManager::SetUpDefaultConfig();
	QVector<olive::core::TimeRange> received;
	olive::Project project;
	project.Initialize();
	olive::Sequence *sequence = CreateSequence(&project);

	QObject::connect(sequence, &olive::Sequence::SubtitlesChanged,
					 [&received](const olive::core::TimeRange &r) {
						 received.append(r);
					 });

	// Invalidations from the subtitle track input are forwarded as a signal
	// (Sequence's override hides the base-class default arguments, so the
	// element and options must be passed explicitly)
	const olive::core::TimeRange subtitle_range(olive::core::rational(1),
												olive::core::rational(2));
	sequence->InvalidateCache(subtitle_range,
							  olive::Sequence::kTrackInputFormat.arg(
								  olive::Track::kSubtitle),
							  -1, olive::Node::InvalidateCacheOptions());

	EXPECT_EQ(received, QVector<olive::core::TimeRange>({ subtitle_range }));

	// Invalidations from other inputs do not emit the signal
	sequence->InvalidateCache(
		olive::core::TimeRange(olive::core::rational(3),
							   olive::core::rational(4)),
		olive::Sequence::kTrackInputFormat.arg(olive::Track::kVideo), -1,
		olive::Node::InvalidateCacheOptions());
	sequence->InvalidateCache(
		olive::core::TimeRange(olive::core::rational(3),
							   olive::core::rational(4)),
		olive::ViewerOutput::kTextureInput, -1,
		olive::Node::InvalidateCacheOptions());

	EXPECT_EQ(received.size(), 1);
}

TEST(Sequence, TrackHeightChangePropagatesThroughTrackList)
{
	olive::ColorManager::SetUpDefaultConfig();
	int emissions = 0;
	int signal_height = 0;
	olive::Project project;
	project.Initialize();
	olive::Sequence *sequence = CreateSequence(&project);
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);

	olive::Track *track = CreateTrack(&project);
	AppendTrackToList(list, track);

	olive::Track *signal_track = nullptr;
	QObject::connect(list, &olive::TrackList::TrackHeightChanged,
					 [&emissions, &signal_track, &signal_height](
						 olive::Track *t, int h) {
						 ++emissions;
						 signal_track = t;
						 signal_height = h;
					 });

	track->SetTrackHeightInPixels(96);

	EXPECT_EQ(emissions, 1);
	EXPECT_EQ(signal_track, track);
	EXPECT_EQ(signal_height, 96);
}
