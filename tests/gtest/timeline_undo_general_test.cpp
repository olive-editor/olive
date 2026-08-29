#include <gtest/gtest.h>

#include <memory>

#include "node/block/clip/clip.h"
#include "node/block/gap/gap.h"
#include "node/block/transition/crossdissolve/crossdissolvetransition.h"
#include "node/color/colormanager/colormanager.h"
#include "node/factory.h"
#include "node/math/math/math.h"
#include "node/math/merge/merge.h"
#include "node/output/track/track.h"
#include "node/output/track/tracklist.h"
#include "node/output/viewer/viewer.h"
#include "node/project.h"
#include "node/project/sequence/sequence.h"
#include "timeline/timelineundogeneral.h"

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

olive::GapBlock *CreateGap(olive::Project *project,
						   const olive::core::rational &length)
{
	auto *gap = new olive::GapBlock();
	gap->setParent(project);
	gap->set_length_and_media_out(length);
	return gap;
}

olive::CrossDissolveTransition *CreateTransition(olive::Project *project,
												 const olive::core::rational &length)
{
	auto *transition = new olive::CrossDissolveTransition();
	transition->setParent(project);
	transition->set_length_and_media_out(length);
	return transition;
}

void AppendTrackToList(olive::TrackList *list, olive::Track *track)
{
	list->ArrayAppend();
	olive::Node::ConnectEdge(track,
							 list->track_input(list->ArraySize() - 1));
}

} // namespace

class TimelineUndoGeneralTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		olive::ColorManager::SetUpDefaultConfig();

		project_ = std::make_unique<olive::Project>();
		project_->Initialize();
	}

	std::unique_ptr<olive::Project> project_;
};

TEST_F(TimelineUndoGeneralTest, BlockResizeCommandChangesLength)
{
	olive::ClipBlock *clip = CreateClip(project_.get(), olive::core::rational(4));

	olive::BlockResizeCommand cmd(clip, olive::core::rational(2));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_EQ(clip->length(), olive::core::rational(2));
	// Resizing from the out point leaves the media in point alone
	EXPECT_EQ(clip->media_in(), olive::core::rational(0));

	cmd.undo_now();
	EXPECT_EQ(clip->length(), olive::core::rational(4));

	// Resizing to zero length is allowed on a detached block
	olive::BlockResizeCommand to_zero(clip, olive::core::rational(0));
	to_zero.redo_now();
	EXPECT_EQ(clip->length(), olive::core::rational(0));
	to_zero.undo_now();
	EXPECT_EQ(clip->length(), olive::core::rational(4));
}

TEST_F(TimelineUndoGeneralTest, BlockResizeWithMediaInCommandShiftsMediaIn)
{
	olive::ClipBlock *clip = CreateClip(project_.get(), olive::core::rational(4));
	ASSERT_EQ(clip->media_in(), olive::core::rational(0));

	olive::BlockResizeWithMediaInCommand cmd(clip, olive::core::rational(2));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_EQ(clip->length(), olive::core::rational(2));
	// Resizing from the in point pushes the media in point forward
	EXPECT_EQ(clip->media_in(), olive::core::rational(2));

	cmd.undo_now();
	EXPECT_EQ(clip->length(), olive::core::rational(4));
	EXPECT_EQ(clip->media_in(), olive::core::rational(0));
}

TEST_F(TimelineUndoGeneralTest, BlockSetMediaInCommandSetsAndRestores)
{
	olive::ClipBlock *clip = CreateClip(project_.get(), olive::core::rational(4));
	ASSERT_EQ(clip->media_in(), olive::core::rational(0));

	olive::BlockSetMediaInCommand cmd(clip, olive::core::rational(3));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_EQ(clip->media_in(), olive::core::rational(3));

	cmd.undo_now();
	EXPECT_EQ(clip->media_in(), olive::core::rational(0));
}

TEST_F(TimelineUndoGeneralTest, BlockEnableDisableCommandToggles)
{
	olive::ClipBlock *clip = CreateClip(project_.get(), olive::core::rational(4));
	ASSERT_TRUE(clip->is_enabled());

	olive::BlockEnableDisableCommand cmd(clip, false);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_FALSE(clip->is_enabled());

	cmd.undo_now();
	EXPECT_TRUE(clip->is_enabled());
}

TEST_F(TimelineUndoGeneralTest, AddTrackCommandConnectsDirectly)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);

	olive::TimelineAddTrackCommand cmd(list, false);
	olive::Track *track = cmd.track();
	ASSERT_NE(track, nullptr);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_EQ(list->GetTrackCount(), 1);
	EXPECT_EQ(list->GetTrackAt(0), track);
	EXPECT_EQ(track->sequence(), sequence);
	EXPECT_EQ(track->type(), olive::Track::kVideo);
	EXPECT_EQ(track->Index(), 0);
	EXPECT_EQ(track->project(), project_.get());
	// The first track connects straight to the sequence's texture input
	EXPECT_TRUE(sequence->IsInputConnected(olive::ViewerOutput::kTextureInput));
	EXPECT_EQ(sequence->GetConnectedTextureOutput(), track);

	cmd.undo_now();
	EXPECT_EQ(list->GetTrackCount(), 0);
	EXPECT_EQ(list->ArraySize(), 0);
	EXPECT_EQ(track->sequence(), nullptr);
	EXPECT_EQ(track->project(), nullptr);
	EXPECT_FALSE(sequence->IsInputConnected(olive::ViewerOutput::kTextureInput));
}

TEST_F(TimelineUndoGeneralTest, AddTrackCommandInsertsVideoMergeNode)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);

	// The first track takes the direct connection
	olive::TimelineAddTrackCommand first(list, false);
	first.redo_now();
	olive::Track *track1 = first.track();
	ASSERT_EQ(sequence->GetConnectedTextureOutput(), track1);

	// Adding another video track with automerge inserts a merge node
	olive::TimelineAddTrackCommand second(list, true);
	olive::Track *track2 = second.track();

	second.redo_now();
	EXPECT_EQ(list->GetTrackCount(), 2);

	olive::Node *merge = sequence->GetConnectedTextureOutput();
	ASSERT_NE(merge, nullptr);
	EXPECT_NE(merge, track1);
	EXPECT_NE(merge, track2);
	EXPECT_EQ(merge->project(), project_.get());
	EXPECT_EQ(olive::NodeInput(merge, olive::MergeNode::kBaseIn)
				  .GetConnectedOutput(),
			  track1);
	EXPECT_EQ(olive::NodeInput(merge, olive::MergeNode::kBlendIn)
				  .GetConnectedOutput(),
			  track2);

	second.undo_now();
	// The direct connection from the first track is restored
	EXPECT_EQ(sequence->GetConnectedTextureOutput(), track1);
	EXPECT_EQ(merge->project(), nullptr);
	EXPECT_EQ(track2->project(), nullptr);
	EXPECT_EQ(list->GetTrackCount(), 1);
}

TEST_F(TimelineUndoGeneralTest, AddTrackCommandMergesAudioWithMathNode)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kAudio);

	olive::TimelineAddTrackCommand first(list, false);
	first.redo_now();
	olive::Track *track1 = first.track();
	ASSERT_EQ(sequence->GetConnectedSampleOutput(), track1);

	olive::TimelineAddTrackCommand second(list, true);
	olive::Track *track2 = second.track();

	second.redo_now();
	EXPECT_EQ(list->GetTrackCount(), 2);

	// Audio tracks are summed with a math (add) node
	olive::Node *math = sequence->GetConnectedSampleOutput();
	ASSERT_NE(math, nullptr);
	EXPECT_NE(math, track1);
	EXPECT_NE(math, track2);
	EXPECT_EQ(math->id(), QStringLiteral("org.olivevideoeditor.Olive.math"));
	EXPECT_EQ(olive::NodeInput(math, olive::MathNode::kParamAIn)
				  .GetConnectedOutput(),
			  track1);
	EXPECT_EQ(olive::NodeInput(math, olive::MathNode::kParamBIn)
				  .GetConnectedOutput(),
			  track2);

	second.undo_now();
	EXPECT_EQ(sequence->GetConnectedSampleOutput(), track1);
	EXPECT_EQ(math->project(), nullptr);
	EXPECT_EQ(list->GetTrackCount(), 1);
}

TEST_F(TimelineUndoGeneralTest, RemoveTrackCommandRemovesAndRestores)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);

	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *clip = CreateClip(project_.get(), olive::core::rational(4));
	track->AppendBlock(clip);
	AppendTrackToList(list, track);
	ASSERT_EQ(track->Index(), 0);

	olive::TimelineRemoveTrackCommand cmd(track);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_EQ(track->project(), nullptr);
	EXPECT_EQ(track->sequence(), nullptr);
	EXPECT_EQ(list->GetTrackCount(), 0);
	EXPECT_EQ(list->ArraySize(), 0);
	// The clip was an exclusive dependency of the track and left the graph too
	EXPECT_EQ(clip->project(), nullptr);

	cmd.undo_now();
	EXPECT_EQ(track->project(), project_.get());
	EXPECT_EQ(clip->project(), project_.get());
	EXPECT_EQ(list->GetTrackCount(), 1);
	EXPECT_EQ(list->ArraySize(), 1);
	EXPECT_EQ(list->GetTrackAt(0), track);
	EXPECT_EQ(track->Index(), 0);
	EXPECT_EQ(track->sequence(), sequence);
}

TEST_F(TimelineUndoGeneralTest, TransitionRemoveCommandRestoresClipLengths)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(1));
	olive::CrossDissolveTransition *transition =
		CreateTransition(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	b->set_media_in(olive::core::rational(1));
	track->AppendBlock(a);
	track->AppendBlock(transition);
	track->AppendBlock(b);
	olive::Node::ConnectEdge(
		a, olive::NodeInput(transition,
							olive::TransitionBlock::kOutBlockInput));
	olive::Node::ConnectEdge(
		b, olive::NodeInput(transition, olive::TransitionBlock::kInBlockInput));
	// Layout: a [0,1], transition [1,3], b [3,6] with media_in 1
	ASSERT_EQ(transition->connected_out_block(), a);
	ASSERT_EQ(transition->connected_in_block(), b);
	ASSERT_TRUE(transition->is_dual_transition());

	olive::TransitionRemoveCommand cmd(transition, true);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	// Both clips reclaim the half of the transition that overlapped them
	EXPECT_EQ(a->length(), olive::core::rational(2));
	EXPECT_EQ(a->in(), olive::core::rational(0));
	EXPECT_EQ(a->out(), olive::core::rational(2));
	EXPECT_EQ(b->length(), olive::core::rational(4));
	EXPECT_EQ(b->media_in(), olive::core::rational(0));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(6));
	EXPECT_EQ(transition->track(), nullptr);
	EXPECT_EQ(transition->project(), nullptr);

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(track->Blocks().at(1), transition);
	EXPECT_EQ(transition->project(), project_.get());
	EXPECT_EQ(transition->in(), olive::core::rational(1));
	EXPECT_EQ(transition->out(), olive::core::rational(3));
	EXPECT_EQ(transition->connected_out_block(), a);
	EXPECT_EQ(transition->connected_in_block(), b);
	EXPECT_EQ(a->length(), olive::core::rational(1));
	EXPECT_EQ(b->length(), olive::core::rational(3));
	EXPECT_EQ(b->media_in(), olive::core::rational(1));
	EXPECT_EQ(b->in(), olive::core::rational(3));
	EXPECT_EQ(b->out(), olive::core::rational(6));
}

TEST_F(TimelineUndoGeneralTest, ReplaceBlockWithGapCreatesGap)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	olive::ClipBlock *c = CreateClip(project_.get(), olive::core::rational(1));
	track->AppendBlock(a);
	track->AppendBlock(b);
	track->AppendBlock(c);
	// Layout: a [0,2], b [2,5], c [5,6]

	olive::TrackReplaceBlockWithGapCommand cmd(track, b);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	olive::Block *gap = track->Blocks().at(1);
	EXPECT_NE(dynamic_cast<olive::GapBlock *>(gap), nullptr);
	EXPECT_EQ(gap->in(), olive::core::rational(2));
	EXPECT_EQ(gap->out(), olive::core::rational(5));
	EXPECT_EQ(b->track(), nullptr);
	EXPECT_EQ(c->in(), olive::core::rational(5));
	EXPECT_EQ(track->track_length(), olive::core::rational(6));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
	EXPECT_EQ(c->in(), olive::core::rational(5));
}

TEST_F(TimelineUndoGeneralTest, ReplaceBlockWithGapExtendsPreviousGap)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::GapBlock *g = CreateGap(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	olive::ClipBlock *c = CreateClip(project_.get(), olive::core::rational(1));
	track->AppendBlock(g);
	track->AppendBlock(b);
	track->AppendBlock(c);
	// Layout: gap [0,2], b [2,5], c [5,6]

	olive::TrackReplaceBlockWithGapCommand cmd(track, b);

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(0), g);
	// The preceding gap grows to absorb the removed block's time
	EXPECT_EQ(g->length(), olive::core::rational(5));
	EXPECT_EQ(g->in(), olive::core::rational(0));
	EXPECT_EQ(g->out(), olive::core::rational(5));
	EXPECT_EQ(c->in(), olive::core::rational(5));
	EXPECT_EQ(c->out(), olive::core::rational(6));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(g->length(), olive::core::rational(2));
	EXPECT_EQ(g->out(), olive::core::rational(2));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
	EXPECT_EQ(c->in(), olive::core::rational(5));
}

TEST_F(TimelineUndoGeneralTest, ReplaceBlockWithGapExtendsNextGap)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(2));
	olive::GapBlock *g = CreateGap(project_.get(), olive::core::rational(2));
	track->AppendBlock(a);
	track->AppendBlock(b);
	track->AppendBlock(g);
	// Layout: a [0,2], b [2,4], gap [4,6]

	olive::TrackReplaceBlockWithGapCommand cmd(track, b);

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(1), g);
	// The following gap grows backwards to absorb the removed block's time
	EXPECT_EQ(g->length(), olive::core::rational(4));
	EXPECT_EQ(g->in(), olive::core::rational(2));
	EXPECT_EQ(g->out(), olive::core::rational(6));
	EXPECT_EQ(a->out(), olive::core::rational(2));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(4));
	EXPECT_EQ(g->length(), olive::core::rational(2));
	EXPECT_EQ(g->in(), olive::core::rational(4));
	EXPECT_EQ(g->out(), olive::core::rational(6));
}

TEST_F(TimelineUndoGeneralTest, ReplaceBlockWithGapMergesSurroundingGaps)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::GapBlock *g1 = CreateGap(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(2));
	olive::GapBlock *g2 = CreateGap(project_.get(), olive::core::rational(2));
	olive::ClipBlock *c = CreateClip(project_.get(), olive::core::rational(2));
	track->AppendBlock(g1);
	track->AppendBlock(b);
	track->AppendBlock(g2);
	track->AppendBlock(c);
	// Layout: gap [0,2], b [2,4], gap [4,6], c [6,8]

	olive::TrackReplaceBlockWithGapCommand cmd(track, b);

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(0), g1);
	// Both surrounding gaps merge into one covering the removed block
	EXPECT_EQ(g1->length(), olive::core::rational(6));
	EXPECT_EQ(g1->in(), olive::core::rational(0));
	EXPECT_EQ(g1->out(), olive::core::rational(6));
	EXPECT_EQ(c->in(), olive::core::rational(6));
	EXPECT_EQ(c->out(), olive::core::rational(8));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 4);
	EXPECT_EQ(track->Blocks().at(0), g1);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(track->Blocks().at(2), g2);
	EXPECT_EQ(track->Blocks().at(3), c);
	EXPECT_EQ(g1->length(), olive::core::rational(2));
	EXPECT_EQ(g2->length(), olive::core::rational(2));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(4));
	EXPECT_EQ(g2->in(), olive::core::rational(4));
	EXPECT_EQ(g2->out(), olive::core::rational(6));
	EXPECT_EQ(c->in(), olive::core::rational(6));
}

TEST_F(TimelineUndoGeneralTest, ReplaceBlockWithGapRemovesLastBlock)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	track->AppendBlock(a);
	track->AppendBlock(b);
	// Layout: a [0,2], b [2,5]

	olive::TrackReplaceBlockWithGapCommand cmd(track, b);

	cmd.redo_now();
	// The last block needs no gap, it is simply removed
	ASSERT_EQ(track->Blocks().size(), 1);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(b->track(), nullptr);
	EXPECT_EQ(track->track_length(), olive::core::rational(2));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
}

TEST_F(TimelineUndoGeneralTest, ReplaceBlockWithGapRemovesPrecedingGapAtEnd)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::GapBlock *g = CreateGap(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	track->AppendBlock(g);
	track->AppendBlock(b);
	// Layout: gap [0,2], b [2,5]

	olive::TrackReplaceBlockWithGapCommand cmd(track, b);

	cmd.redo_now();
	// Removing the last block also removes the now-pointless gap before it
	EXPECT_TRUE(track->Blocks().isEmpty());
	EXPECT_EQ(track->track_length(), olive::core::rational(0));
	EXPECT_EQ(g->track(), nullptr);
	EXPECT_EQ(b->track(), nullptr);

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(0), g);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(g->in(), olive::core::rational(0));
	EXPECT_EQ(g->out(), olive::core::rational(2));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
}

TEST_F(TimelineUndoGeneralTest, InsertGapsSplitsClipAndInsertsGap)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(4));
	track->AppendBlock(a);
	track->AppendBlock(b);
	AppendTrackToList(list, track);
	// Layout: a [0,2], b [2,6]

	olive::TrackListInsertGaps cmd(list, olive::core::rational(3),
								   olive::core::rational(2));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	// b is split at the insert point and a gap goes between the halves
	ASSERT_EQ(track->Blocks().size(), 4);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(b->length(), olive::core::rational(1));
	EXPECT_EQ(b->out(), olive::core::rational(3));
	olive::Block *gap = track->Blocks().at(2);
	EXPECT_NE(dynamic_cast<olive::GapBlock *>(gap), nullptr);
	EXPECT_EQ(gap->in(), olive::core::rational(3));
	EXPECT_EQ(gap->out(), olive::core::rational(5));
	olive::Block *second_half = track->Blocks().at(3);
	EXPECT_EQ(second_half->in(), olive::core::rational(5));
	EXPECT_EQ(second_half->out(), olive::core::rational(8));
	EXPECT_EQ(track->track_length(), olive::core::rational(8));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(b->length(), olive::core::rational(4));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(6));
	EXPECT_EQ(track->track_length(), olive::core::rational(6));
}

TEST_F(TimelineUndoGeneralTest, InsertGapsExtendsExistingGap)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);
	olive::Track *track = CreateTrack(project_.get());
	olive::GapBlock *g = CreateGap(project_.get(), olive::core::rational(4));
	olive::ClipBlock *c = CreateClip(project_.get(), olive::core::rational(2));
	track->AppendBlock(g);
	track->AppendBlock(c);
	AppendTrackToList(list, track);
	// Layout: gap [0,4], c [4,6]

	olive::TrackListInsertGaps cmd(list, olive::core::rational(3),
								   olive::core::rational(2));

	cmd.redo_now();
	// A gap already at the insert point simply grows
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(g->length(), olive::core::rational(6));
	EXPECT_EQ(g->out(), olive::core::rational(6));
	EXPECT_EQ(c->in(), olive::core::rational(6));
	EXPECT_EQ(c->out(), olive::core::rational(8));

	cmd.undo_now();
	EXPECT_EQ(g->length(), olive::core::rational(4));
	EXPECT_EQ(g->out(), olive::core::rational(4));
	EXPECT_EQ(c->in(), olive::core::rational(4));
	EXPECT_EQ(c->out(), olive::core::rational(6));
}

TEST_F(TimelineUndoGeneralTest, InsertGapsSkipsLockedTracks)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);

	olive::Track *t1 = CreateTrack(project_.get());
	t1->AppendBlock(CreateClip(project_.get(), olive::core::rational(4)));
	AppendTrackToList(list, t1);

	olive::Track *t2 = CreateTrack(project_.get());
	olive::ClipBlock *c = CreateClip(project_.get(), olive::core::rational(4));
	t2->AppendBlock(c);
	AppendTrackToList(list, t2);
	t2->SetLocked(true);
	// Layout: t1 [0,4] / t2 [0,4]

	olive::TrackListInsertGaps cmd(list, olive::core::rational(2),
								   olive::core::rational(2));

	cmd.redo_now();
	EXPECT_EQ(t1->track_length(), olive::core::rational(6));
	// The locked track is untouched
	EXPECT_EQ(t2->Blocks().size(), 1);
	EXPECT_EQ(t2->track_length(), olive::core::rational(4));

	cmd.undo_now();
	EXPECT_EQ(t1->track_length(), olive::core::rational(4));
	EXPECT_EQ(t2->track_length(), olive::core::rational(4));
}

TEST_F(TimelineUndoGeneralTest, InsertGapsAtOrBeyondEndDoesNothing)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	track->AppendBlock(a);
	AppendTrackToList(list, track);
	// Layout: a [0,2]

	// Exactly at the end of the last block no gap is needed
	olive::TrackListInsertGaps at_end(list, olive::core::rational(2),
									  olive::core::rational(2));
	at_end.redo_now();
	ASSERT_EQ(track->Blocks().size(), 1);
	EXPECT_EQ(track->track_length(), olive::core::rational(2));
	at_end.undo_now();
	EXPECT_EQ(track->track_length(), olive::core::rational(2));

	// Beyond all content there is nothing to split or extend
	olive::TrackListInsertGaps beyond(list, olive::core::rational(5),
									  olive::core::rational(2));
	beyond.redo_now();
	ASSERT_EQ(track->Blocks().size(), 1);
	EXPECT_EQ(track->track_length(), olive::core::rational(2));
	beyond.undo_now();
	EXPECT_EQ(track->track_length(), olive::core::rational(2));
}

TEST_F(TimelineUndoGeneralTest, AddDefaultTransitionAddsInAndOutTransitions)
{
	olive::NodeFactory::Initialize();

	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *c = CreateClip(project_.get(), olive::core::rational(4));
	track->AppendBlock(c);
	AppendTrackToList(list, track);
	// Layout: c [0,4]

	olive::TimelineAddDefaultTransitionCommand cmd(
		{ c }, olive::core::rational(1, 30));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();

	// A lone clip gets an in transition and an out transition, each one second
	ASSERT_EQ(track->Blocks().size(), 3);
	auto *in_transition =
		dynamic_cast<olive::TransitionBlock *>(track->Blocks().at(0));
	auto *out_transition =
		dynamic_cast<olive::TransitionBlock *>(track->Blocks().at(2));
	ASSERT_NE(in_transition, nullptr);
	ASSERT_NE(out_transition, nullptr);

	EXPECT_EQ(in_transition->in(), olive::core::rational(0));
	EXPECT_EQ(in_transition->out(), olive::core::rational(1));
	EXPECT_EQ(in_transition->connected_in_block(), c);
	EXPECT_EQ(in_transition->connected_out_block(), nullptr);
	EXPECT_FALSE(in_transition->is_dual_transition());

	EXPECT_EQ(c->length(), olive::core::rational(2));
	EXPECT_EQ(c->media_in(), olive::core::rational(1));
	EXPECT_EQ(c->in(), olive::core::rational(1));
	EXPECT_EQ(c->out(), olive::core::rational(3));

	EXPECT_EQ(out_transition->in(), olive::core::rational(3));
	EXPECT_EQ(out_transition->out(), olive::core::rational(4));
	EXPECT_EQ(out_transition->connected_out_block(), c);
	EXPECT_EQ(out_transition->connected_in_block(), nullptr);

	// The total length of the track is unchanged
	EXPECT_EQ(track->track_length(), olive::core::rational(4));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 1);
	EXPECT_EQ(track->Blocks().at(0), c);
	EXPECT_EQ(c->length(), olive::core::rational(4));
	EXPECT_EQ(c->media_in(), olive::core::rational(0));
	EXPECT_EQ(c->in(), olive::core::rational(0));
	EXPECT_EQ(c->out(), olive::core::rational(4));
	EXPECT_EQ(in_transition->project(), nullptr);
	EXPECT_EQ(out_transition->project(), nullptr);
}

TEST_F(TimelineUndoGeneralTest, AddDefaultTransitionAddsDualTransition)
{
	olive::NodeFactory::Initialize();

	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(4));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(4));
	track->AppendBlock(a);
	track->AppendBlock(b);
	AppendTrackToList(list, track);
	// Layout: a [0,4], b [4,8]

	olive::TimelineAddDefaultTransitionCommand cmd(
		{ a, b }, olive::core::rational(1, 30));

	cmd.redo_now();

	// Adjacent clips get an in transition on a, a dual transition between
	// them, and an out transition on b
	ASSERT_EQ(track->Blocks().size(), 5);
	auto *in_transition =
		dynamic_cast<olive::TransitionBlock *>(track->Blocks().at(0));
	auto *dual_transition =
		dynamic_cast<olive::TransitionBlock *>(track->Blocks().at(2));
	auto *out_transition =
		dynamic_cast<olive::TransitionBlock *>(track->Blocks().at(4));
	ASSERT_NE(in_transition, nullptr);
	ASSERT_NE(dual_transition, nullptr);
	ASSERT_NE(out_transition, nullptr);

	EXPECT_TRUE(dual_transition->is_dual_transition());
	EXPECT_EQ(dual_transition->connected_out_block(), a);
	EXPECT_EQ(dual_transition->connected_in_block(), b);
	// A centered dual transition overlaps each clip by half its length
	EXPECT_EQ(dual_transition->in_offset(), olive::core::rational(1, 2));
	EXPECT_EQ(dual_transition->out_offset(), olive::core::rational(1, 2));

	EXPECT_EQ(a->length(), olive::core::rational(5, 2));
	EXPECT_EQ(a->in(), olive::core::rational(1));
	EXPECT_EQ(a->out(), olive::core::rational(7, 2));
	EXPECT_EQ(b->length(), olive::core::rational(5, 2));
	EXPECT_EQ(b->media_in(), olive::core::rational(1, 2));
	EXPECT_EQ(b->in(), olive::core::rational(9, 2));
	EXPECT_EQ(b->out(), olive::core::rational(7));

	EXPECT_EQ(dual_transition->in(), olive::core::rational(7, 2));
	EXPECT_EQ(dual_transition->out(), olive::core::rational(9, 2));
	EXPECT_EQ(track->track_length(), olive::core::rational(8));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(a->length(), olive::core::rational(4));
	EXPECT_EQ(a->media_in(), olive::core::rational(0));
	EXPECT_EQ(a->in(), olive::core::rational(0));
	EXPECT_EQ(a->out(), olive::core::rational(4));
	EXPECT_EQ(b->length(), olive::core::rational(4));
	EXPECT_EQ(b->media_in(), olive::core::rational(0));
	EXPECT_EQ(b->in(), olive::core::rational(4));
	EXPECT_EQ(b->out(), olive::core::rational(8));
}

TEST_F(TimelineUndoGeneralTest, AddDefaultTransitionEmptyClipListIsHarmless)
{
	olive::TimelineAddDefaultTransitionCommand cmd(
		{}, olive::core::rational(1, 30));
	EXPECT_EQ(cmd.GetRelevantProject(), nullptr);

	// redo/undo on an empty command must be harmless no-ops
	cmd.redo_now();
	cmd.undo_now();
}
