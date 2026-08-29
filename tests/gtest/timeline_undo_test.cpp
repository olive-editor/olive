#include <gtest/gtest.h>

#include <memory>

#include "node/block/clip/clip.h"
#include "node/block/gap/gap.h"
#include "node/block/transition/crossdissolve/crossdissolvetransition.h"
#include "node/color/colormanager/colormanager.h"
#include "node/math/math/math.h"
#include "node/output/track/track.h"
#include "node/output/track/tracklist.h"
#include "node/project.h"
#include "node/project/sequence/sequence.h"
#include "timeline/timelineundocommon.h"
#include "timeline/timelineundopointer.h"
#include "timeline/timelineundoripple.h"
#include "timeline/timelineundosplit.h"
#include "timeline/timelineundotrack.h"
#include "timeline/timelineundoworkarea.h"

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

void AppendTrackToList(olive::TrackList *list, olive::Track *track)
{
	list->ArrayAppend();
	olive::Node::ConnectEdge(track,
							 list->track_input(list->ArraySize() - 1));
}

} // namespace

class TimelineUndoTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		olive::ColorManager::SetUpDefaultConfig();

		project_ = std::make_unique<olive::Project>();
		project_->Initialize();
	}

	std::unique_ptr<olive::Project> project_;
};

//
// timelineundotrack.h
//
TEST_F(TimelineUndoTest, RippleRemoveBlockCommandRemovesAndRestores)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	olive::ClipBlock *c = CreateClip(project_.get(), olive::core::rational(1));
	track->AppendBlock(a);
	track->AppendBlock(b);
	track->AppendBlock(c);
	// Layout: a [0,2], b [2,5], c [5,6]

	olive::TrackRippleRemoveBlockCommand cmd(track, b);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(track->Blocks().at(1), c);
	EXPECT_EQ(b->track(), nullptr);
	EXPECT_EQ(c->in(), olive::core::rational(2));
	EXPECT_EQ(c->out(), olive::core::rational(3));
	EXPECT_EQ(track->track_length(), olive::core::rational(3));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(b->track(), track);
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
	EXPECT_EQ(c->in(), olive::core::rational(5));
	EXPECT_EQ(track->track_length(), olive::core::rational(6));
}

TEST_F(TimelineUndoTest, PrependBlockCommandInsertsAndRemoves)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(2));
	track->AppendBlock(b);

	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(1));
	olive::TrackPrependBlockCommand cmd(track, a);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(a->in(), olive::core::rational(0));
	EXPECT_EQ(a->out(), olive::core::rational(1));
	EXPECT_EQ(b->in(), olive::core::rational(1));
	EXPECT_EQ(b->out(), olive::core::rational(3));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 1);
	EXPECT_EQ(track->Blocks().at(0), b);
	EXPECT_EQ(a->track(), nullptr);
	EXPECT_EQ(b->in(), olive::core::rational(0));
	EXPECT_EQ(b->out(), olive::core::rational(2));
}

TEST_F(TimelineUndoTest, InsertBlockAfterCommandInsertsAndRemoves)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(1));
	olive::ClipBlock *c = CreateClip(project_.get(), olive::core::rational(1));
	track->AppendBlock(a);
	track->AppendBlock(c);

	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(2));
	olive::TrackInsertBlockAfterCommand cmd(track, b, a);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(b->in(), olive::core::rational(1));
	EXPECT_EQ(b->out(), olive::core::rational(3));
	EXPECT_EQ(c->in(), olive::core::rational(3));
	EXPECT_EQ(track->track_length(), olive::core::rational(4));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(b->track(), nullptr);
	EXPECT_EQ(c->in(), olive::core::rational(1));
	EXPECT_EQ(track->track_length(), olive::core::rational(2));
}

TEST_F(TimelineUndoTest, ReplaceBlockCommandSwapsAndRestores)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	track->AppendBlock(a);
	track->AppendBlock(b);

	olive::ClipBlock *r = CreateClip(project_.get(), olive::core::rational(3));
	olive::TrackReplaceBlockCommand cmd(track, b, r);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(1), r);
	EXPECT_EQ(b->track(), nullptr);
	EXPECT_EQ(r->in(), olive::core::rational(2));
	EXPECT_EQ(r->out(), olive::core::rational(5));
	EXPECT_EQ(track->track_length(), olive::core::rational(5));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(r->track(), nullptr);
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
}

//
// timelineundoworkarea.h
//
TEST_F(TimelineUndoTest, WorkareaSetEnabledCommandToggles)
{
	olive::TimelineWorkArea workarea;
	ASSERT_FALSE(workarea.enabled());

	olive::WorkareaSetEnabledCommand cmd(project_.get(), &workarea, true);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_TRUE(workarea.enabled());

	cmd.undo_now();
	EXPECT_FALSE(workarea.enabled());
}

TEST_F(TimelineUndoTest, WorkareaSetRangeCommandSetsAndRestores)
{
	olive::TimelineWorkArea workarea;
	const olive::core::TimeRange old_range(olive::core::rational(2),
										   olive::core::rational(6));
	const olive::core::TimeRange new_range(olive::core::rational(3),
										   olive::core::rational(9));
	workarea.set_range(old_range);

	// The two-argument form captures the workarea's current range as the old one
	olive::WorkareaSetRangeCommand cmd(&workarea, new_range);

	cmd.redo_now();
	EXPECT_EQ(workarea.range(), new_range);

	cmd.undo_now();
	EXPECT_EQ(workarea.range(), old_range);
}

TEST_F(TimelineUndoTest, WorkareaSetRangeCommandExplicitOldRange)
{
	olive::TimelineWorkArea workarea;
	const olive::core::TimeRange old_range(olive::core::rational(0),
										   olive::core::rational(10));
	const olive::core::TimeRange new_range(olive::core::rational(4),
										   olive::core::rational(5));
	workarea.set_range(old_range);

	olive::WorkareaSetRangeCommand cmd(&workarea, new_range, old_range);

	cmd.redo_now();
	EXPECT_EQ(workarea.range(), new_range);
	EXPECT_EQ(workarea.in(), olive::core::rational(4));
	EXPECT_EQ(workarea.out(), olive::core::rational(5));

	cmd.undo_now();
	EXPECT_EQ(workarea.range(), old_range);
}

//
// timelineundocommon.h
//
TEST_F(TimelineUndoTest, NodeCanBeRemovedReflectsConnections)
{
	olive::ClipBlock *clip = CreateClip(project_.get(), olive::core::rational(2));

	// An unconnected node has no output connections and can be removed
	EXPECT_TRUE(olive::NodeCanBeRemoved(clip));

	olive::Track *track = CreateTrack(project_.get());
	track->AppendBlock(clip);
	EXPECT_FALSE(olive::NodeCanBeRemoved(clip));

	track->RippleRemoveBlock(clip);
	EXPECT_TRUE(olive::NodeCanBeRemoved(clip));
}

TEST_F(TimelineUndoTest, CreateAndRunRemoveCommandRemovesFromGraph)
{
	auto *node = new olive::MathNode();
	node->setParent(project_.get());
	ASSERT_EQ(node->project(), project_.get());

	olive::UndoCommand *cmd = olive::CreateAndRunRemoveCommand(node);
	EXPECT_EQ(node->project(), nullptr);
	EXPECT_FALSE(project_->nodes().contains(node));

	cmd->undo_now();
	EXPECT_EQ(node->project(), project_.get());
	EXPECT_TRUE(project_->nodes().contains(node));

	delete cmd;
}

//
// timelineundosplit.h
//
TEST_F(TimelineUndoTest, BlockSplitCommandSplitsAndMerges)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(4));
	track->AppendBlock(a);
	// Layout: a [0,4]

	olive::BlockSplitCommand cmd(a, olive::core::rational(1));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();

	olive::Block *split = cmd.new_block();
	ASSERT_NE(split, nullptr);
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(track->Blocks().at(1), split);

	EXPECT_EQ(a->length(), olive::core::rational(1));
	EXPECT_EQ(a->in(), olive::core::rational(0));
	EXPECT_EQ(a->out(), olive::core::rational(1));

	EXPECT_EQ(split->length(), olive::core::rational(3));
	EXPECT_EQ(split->in(), olive::core::rational(1));
	EXPECT_EQ(split->out(), olive::core::rational(4));
	// The second half's media in point is offset by the split time
	EXPECT_EQ(static_cast<olive::ClipBlock *>(split)->media_in(),
			  olive::core::rational(1));

	EXPECT_EQ(track->track_length(), olive::core::rational(4));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 1);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(a->length(), olive::core::rational(4));
	EXPECT_EQ(a->in(), olive::core::rational(0));
	EXPECT_EQ(a->out(), olive::core::rational(4));
	EXPECT_EQ(split->track(), nullptr);
}

TEST_F(TimelineUndoTest, BlockSplitCommandMovesOutTransitionToNewBlock)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(4));
	auto *transition = new olive::CrossDissolveTransition();
	transition->setParent(project_.get());
	transition->set_length_and_media_out(olive::core::rational(2));
	track->AppendBlock(a);
	track->AppendBlock(transition);
	olive::Node::ConnectEdge(
		a, olive::NodeInput(transition, olive::TransitionBlock::kOutBlockInput));
	ASSERT_EQ(transition->connected_out_block(), a);
	// Layout: a [0,4], transition [4,6]

	olive::BlockSplitCommand cmd(a, olive::core::rational(2));
	cmd.redo_now();

	olive::Block *split = cmd.new_block();
	ASSERT_NE(split, nullptr);
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(split->in(), olive::core::rational(2));
	EXPECT_EQ(split->out(), olive::core::rational(4));
	EXPECT_EQ(transition->in(), olive::core::rational(4));

	// The out transition moved from the original block to the split block
	EXPECT_EQ(transition->connected_out_block(), split);
	EXPECT_EQ(olive::NodeInput(transition,
							   olive::TransitionBlock::kOutBlockInput)
				  .GetConnectedOutput(),
			  split);

	cmd.undo_now();

	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(a->length(), olive::core::rational(4));
	EXPECT_EQ(transition->connected_out_block(), a);
	EXPECT_EQ(transition->in(), olive::core::rational(4));
}

TEST_F(TimelineUndoTest, BlockSplitPreservingLinksCommandSplitsLinkedBlocks)
{
	olive::Track *video_track = CreateTrack(project_.get());
	olive::Track *audio_track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(4));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(4));
	video_track->AppendBlock(a);
	audio_track->AppendBlock(b);
	olive::Node::Link(a, b);

	olive::BlockSplitPreservingLinksCommand cmd(
		{ a, b }, { olive::core::rational(2) });
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();

	ASSERT_EQ(video_track->Blocks().size(), 2);
	ASSERT_EQ(audio_track->Blocks().size(), 2);
	EXPECT_EQ(a->length(), olive::core::rational(2));
	EXPECT_EQ(b->length(), olive::core::rational(2));

	olive::Block *a_split = cmd.GetSplit(a, 0);
	olive::Block *b_split = cmd.GetSplit(b, 0);
	ASSERT_NE(a_split, nullptr);
	ASSERT_NE(b_split, nullptr);
	EXPECT_EQ(a_split->in(), olive::core::rational(2));
	EXPECT_EQ(b_split->in(), olive::core::rational(2));

	// The original link survives and the new halves are linked together too
	EXPECT_TRUE(olive::Node::AreLinked(a, b));
	EXPECT_TRUE(olive::Node::AreLinked(a_split, b_split));

	// Invalid lookups return null instead of crashing
	EXPECT_EQ(cmd.GetSplit(a, 1), nullptr);
	EXPECT_EQ(cmd.GetSplit(a, -1), nullptr);
	olive::ClipBlock *stray =
		CreateClip(project_.get(), olive::core::rational(1));
	EXPECT_EQ(cmd.GetSplit(stray, 0), nullptr);

	cmd.undo_now();

	ASSERT_EQ(video_track->Blocks().size(), 1);
	ASSERT_EQ(audio_track->Blocks().size(), 1);
	EXPECT_EQ(a->length(), olive::core::rational(4));
	EXPECT_EQ(b->length(), olive::core::rational(4));
	EXPECT_TRUE(olive::Node::AreLinked(a, b));
}

TEST_F(TimelineUndoTest, TrackSplitAtTimeCommandSplitsContainingBlock)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	track->AppendBlock(a);
	track->AppendBlock(b);
	// Layout: a [0,2], b [2,5]

	olive::TrackSplitAtTimeCommand cmd(track, olive::core::rational(3));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(b->length(), olive::core::rational(1));
	EXPECT_EQ(b->out(), olive::core::rational(3));
	EXPECT_EQ(track->Blocks().at(2)->in(), olive::core::rational(3));
	EXPECT_EQ(track->Blocks().at(2)->out(), olive::core::rational(5));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(b->length(), olive::core::rational(3));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
}

TEST_F(TimelineUndoTest, TrackSplitAtTimeCommandNoOpOutsideBlocks)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	track->AppendBlock(a);
	track->AppendBlock(b);
	// Layout: a [0,2], b [2,5]

	// A time exactly on a block boundary is not contained by any block
	olive::TrackSplitAtTimeCommand on_edge(track, olive::core::rational(2));
	on_edge.redo_now();
	EXPECT_EQ(track->Blocks().size(), 2);
	on_edge.undo_now();
	EXPECT_EQ(track->Blocks().size(), 2);

	// A time past the end of the track contains nothing either
	olive::TrackSplitAtTimeCommand past_end(track, olive::core::rational(10));
	past_end.redo_now();
	EXPECT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(b->length(), olive::core::rational(3));
	past_end.undo_now();
	EXPECT_EQ(track->Blocks().size(), 2);
}

//
// timelineundoripple.h
//
TEST_F(TimelineUndoTest, RippleRemoveAreaRemovesMiddleBlock)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	olive::ClipBlock *c = CreateClip(project_.get(), olive::core::rational(1));
	track->AppendBlock(a);
	track->AppendBlock(b);
	track->AppendBlock(c);
	// Layout: a [0,2], b [2,5], c [5,6]

	olive::TrackRippleRemoveAreaCommand cmd(
		track, olive::core::TimeRange(olive::core::rational(2),
									  olive::core::rational(5)));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(track->Blocks().at(1), c);
	EXPECT_EQ(c->in(), olive::core::rational(2));
	EXPECT_EQ(c->out(), olive::core::rational(3));
	EXPECT_EQ(track->track_length(), olive::core::rational(3));

	// The removed block was taken out of the graph entirely
	EXPECT_EQ(b->track(), nullptr);
	EXPECT_EQ(b->project(), nullptr);

	// An insertion would go after the block preceding the removed area
	EXPECT_EQ(cmd.GetInsertionIndex(), a);

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(b->project(), project_.get());
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
	EXPECT_EQ(c->in(), olive::core::rational(5));
	EXPECT_EQ(track->track_length(), olive::core::rational(6));
}

TEST_F(TimelineUndoTest, RippleRemoveAreaRemovesMultipleBlocks)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(1));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(1));
	olive::ClipBlock *c = CreateClip(project_.get(), olive::core::rational(1));
	olive::ClipBlock *d = CreateClip(project_.get(), olive::core::rational(1));
	track->AppendBlock(a);
	track->AppendBlock(b);
	track->AppendBlock(c);
	track->AppendBlock(d);
	// Layout: a [0,1], b [1,2], c [2,3], d [3,4]

	olive::TrackRippleRemoveAreaCommand cmd(
		track, olive::core::TimeRange(olive::core::rational(1),
									  olive::core::rational(3)));

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(track->Blocks().at(1), d);
	EXPECT_EQ(d->in(), olive::core::rational(1));
	EXPECT_EQ(d->out(), olive::core::rational(2));
	EXPECT_EQ(track->track_length(), olive::core::rational(2));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 4);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(track->Blocks().at(2), c);
	EXPECT_EQ(track->Blocks().at(3), d);
	EXPECT_EQ(b->in(), olive::core::rational(1));
	EXPECT_EQ(c->in(), olive::core::rational(2));
	EXPECT_EQ(d->in(), olive::core::rational(3));
	EXPECT_EQ(track->track_length(), olive::core::rational(4));
}

TEST_F(TimelineUndoTest, RippleRemoveAreaTrimsBothEnds)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(4));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(4));
	track->AppendBlock(a);
	track->AppendBlock(b);
	// Layout: a [0,4], b [4,8]

	olive::TrackRippleRemoveAreaCommand cmd(
		track, olive::core::TimeRange(olive::core::rational(2),
									  olive::core::rational(6)));

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	// a is out-trimmed to the range start, b is in-trimmed to the range end
	EXPECT_EQ(a->length(), olive::core::rational(2));
	EXPECT_EQ(a->in(), olive::core::rational(0));
	EXPECT_EQ(a->out(), olive::core::rational(2));
	EXPECT_EQ(b->length(), olive::core::rational(2));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(4));
	EXPECT_EQ(track->track_length(), olive::core::rational(4));

	cmd.undo_now();
	EXPECT_EQ(a->length(), olive::core::rational(4));
	EXPECT_EQ(b->length(), olive::core::rational(4));
	EXPECT_EQ(a->in(), olive::core::rational(0));
	EXPECT_EQ(a->out(), olive::core::rational(4));
	EXPECT_EQ(b->in(), olive::core::rational(4));
	EXPECT_EQ(b->out(), olive::core::rational(8));
}

TEST_F(TimelineUndoTest, RippleRemoveAreaSplicesBlock)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(10));
	track->AppendBlock(a);
	// Layout: a [0,10]

	olive::TrackRippleRemoveAreaCommand cmd(
		track, olive::core::TimeRange(olive::core::rational(3),
									  olive::core::rational(6)));

	cmd.redo_now();

	// The block is split around the removed range and the second half is
	// in-trimmed by the range length
	olive::Block *spliced = cmd.GetSplicedBlock();
	ASSERT_NE(spliced, nullptr);
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(a->length(), olive::core::rational(3));
	EXPECT_EQ(a->out(), olive::core::rational(3));
	EXPECT_EQ(spliced->in(), olive::core::rational(3));
	EXPECT_EQ(spliced->out(), olive::core::rational(7));
	EXPECT_EQ(spliced->length(), olive::core::rational(4));
	EXPECT_EQ(static_cast<olive::ClipBlock *>(spliced)->media_in(),
			  olive::core::rational(6));
	EXPECT_EQ(track->track_length(), olive::core::rational(7));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 1);
	EXPECT_EQ(a->length(), olive::core::rational(10));
	EXPECT_EQ(a->in(), olive::core::rational(0));
	EXPECT_EQ(a->out(), olive::core::rational(10));
}

TEST_F(TimelineUndoTest, RippleRemoveAreaTrimsGapWhenSplittingGapsDisabled)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::GapBlock *g = CreateGap(project_.get(), olive::core::rational(5));
	track->AppendBlock(g);
	// Layout: gap [0,5]

	olive::TrackRippleRemoveAreaCommand cmd(
		track, olive::core::TimeRange(olive::core::rational(2),
									  olive::core::rational(4)));
	ASSERT_EQ(cmd.GetSplicedBlock(), nullptr);

	cmd.redo_now();
	// Gaps are not spliced by default, the gap is just trimmed by the range
	ASSERT_EQ(track->Blocks().size(), 1);
	EXPECT_EQ(g->length(), olive::core::rational(3));
	EXPECT_EQ(g->in(), olive::core::rational(0));
	EXPECT_EQ(g->out(), olive::core::rational(3));
	EXPECT_EQ(cmd.GetSplicedBlock(), nullptr);

	cmd.undo_now();
	EXPECT_EQ(g->length(), olive::core::rational(5));
	EXPECT_EQ(g->out(), olive::core::rational(5));
}

TEST_F(TimelineUndoTest, RippleRemoveAreaNoOpOnEmptyRange)
{
	olive::Track *track = CreateTrack(project_.get());

	// Empty track: nothing to remove
	olive::TrackRippleRemoveAreaCommand empty_track(
		track, olive::core::TimeRange(olive::core::rational(0),
									  olive::core::rational(5)));
	empty_track.redo_now();
	EXPECT_TRUE(track->Blocks().isEmpty());
	empty_track.undo_now();
	EXPECT_TRUE(track->Blocks().isEmpty());

	// Range fully beyond the track's content: also nothing to remove
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	track->AppendBlock(a);
	olive::TrackRippleRemoveAreaCommand past_end(
		track, olive::core::TimeRange(olive::core::rational(5),
									  olive::core::rational(7)));
	past_end.redo_now();
	ASSERT_EQ(track->Blocks().size(), 1);
	EXPECT_EQ(a->length(), olive::core::rational(2));
	EXPECT_EQ(track->track_length(), olive::core::rational(2));
	past_end.undo_now();
	EXPECT_EQ(track->track_length(), olive::core::rational(2));
}

TEST_F(TimelineUndoTest, TrackListRippleRemoveAreaAffectsAllTracks)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);

	olive::Track *t1 = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	t1->AppendBlock(a);
	t1->AppendBlock(b);
	AppendTrackToList(list, t1);

	olive::Track *t2 = CreateTrack(project_.get());
	olive::ClipBlock *c = CreateClip(project_.get(), olive::core::rational(3));
	olive::ClipBlock *d = CreateClip(project_.get(), olive::core::rational(3));
	t2->AppendBlock(c);
	t2->AppendBlock(d);
	AppendTrackToList(list, t2);
	// t1: a [0,2], b [2,5] / t2: c [0,3], d [3,6]

	olive::TrackListRippleRemoveAreaCommand cmd(list, olive::core::rational(2),
												olive::core::rational(4));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	// t1: b is in-trimmed by the range
	EXPECT_EQ(b->length(), olive::core::rational(1));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(3));
	// t2: c is out-trimmed and d is in-trimmed
	EXPECT_EQ(c->length(), olive::core::rational(2));
	EXPECT_EQ(d->length(), olive::core::rational(2));
	EXPECT_EQ(d->in(), olive::core::rational(2));
	EXPECT_EQ(d->out(), olive::core::rational(4));

	cmd.undo_now();
	EXPECT_EQ(b->length(), olive::core::rational(3));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
	EXPECT_EQ(c->length(), olive::core::rational(3));
	EXPECT_EQ(d->length(), olive::core::rational(3));
	EXPECT_EQ(d->in(), olive::core::rational(3));
	EXPECT_EQ(d->out(), olive::core::rational(6));
}

TEST_F(TimelineUndoTest, TrackListRippleRemoveAreaSkipsLockedTracks)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);

	olive::Track *t1 = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	t1->AppendBlock(a);
	t1->AppendBlock(b);
	AppendTrackToList(list, t1);

	olive::Track *t2 = CreateTrack(project_.get());
	olive::ClipBlock *c = CreateClip(project_.get(), olive::core::rational(3));
	t2->AppendBlock(c);
	AppendTrackToList(list, t2);
	t2->SetLocked(true);

	olive::TrackListRippleRemoveAreaCommand cmd(list, olive::core::rational(2),
												olive::core::rational(4));

	cmd.redo_now();
	EXPECT_EQ(b->length(), olive::core::rational(1));
	// The locked track is untouched
	EXPECT_EQ(c->length(), olive::core::rational(3));
	EXPECT_EQ(c->in(), olive::core::rational(0));
	EXPECT_EQ(c->out(), olive::core::rational(3));

	cmd.undo_now();
	EXPECT_EQ(b->length(), olive::core::rational(3));
	EXPECT_EQ(c->length(), olive::core::rational(3));
}

TEST_F(TimelineUndoTest, TimelineRippleRemoveAreaAffectsAllTrackTypes)
{
	olive::Sequence *sequence = CreateSequence(project_.get());

	olive::Track *video = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	video->AppendBlock(a);
	video->AppendBlock(b);
	AppendTrackToList(sequence->track_list(olive::Track::kVideo), video);

	olive::Track *audio = CreateTrack(project_.get());
	olive::ClipBlock *c = CreateClip(project_.get(), olive::core::rational(3));
	audio->AppendBlock(c);
	AppendTrackToList(sequence->track_list(olive::Track::kAudio), audio);
	// video: a [0,2], b [2,5] / audio: c [0,3]

	olive::TimelineRippleRemoveAreaCommand cmd(sequence,
											   olive::core::rational(1),
											   olive::core::rational(3));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_EQ(a->length(), olive::core::rational(1));
	EXPECT_EQ(b->length(), olive::core::rational(2));
	EXPECT_EQ(b->in(), olive::core::rational(1));
	EXPECT_EQ(b->out(), olive::core::rational(3));
	EXPECT_EQ(c->length(), olive::core::rational(1));
	EXPECT_EQ(c->out(), olive::core::rational(1));

	cmd.undo_now();
	EXPECT_EQ(a->length(), olive::core::rational(2));
	EXPECT_EQ(b->length(), olive::core::rational(3));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
	EXPECT_EQ(c->length(), olive::core::rational(3));
	EXPECT_EQ(c->out(), olive::core::rational(3));
}

TEST_F(TimelineUndoTest, RippleToolCommandTrimsBlockAndRipples)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	track->AppendBlock(a);
	track->AppendBlock(b);
	AppendTrackToList(list, track);
	// Layout: a [0,2], b [2,5]

	QHash<olive::Track *, olive::TrackListRippleToolCommand::RippleInfo> info;
	info.insert(track, { a, false });
	olive::TrackListRippleToolCommand cmd(list, info, olive::core::rational(1),
										  olive::Timeline::kTrimOut);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	// a extends by one, pushing b later
	EXPECT_EQ(a->length(), olive::core::rational(3));
	EXPECT_EQ(a->out(), olive::core::rational(3));
	EXPECT_EQ(b->in(), olive::core::rational(3));
	EXPECT_EQ(b->out(), olive::core::rational(6));
	EXPECT_EQ(track->track_length(), olive::core::rational(6));

	cmd.undo_now();
	EXPECT_EQ(a->length(), olive::core::rational(2));
	EXPECT_EQ(a->out(), olive::core::rational(2));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
}

TEST_F(TimelineUndoTest, RippleToolCommandAppendsGap)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	track->AppendBlock(a);
	track->AppendBlock(b);
	AppendTrackToList(list, track);
	// Layout: a [0,2], b [2,5]

	// Rather than resizing a block, a gap of the movement length is inserted
	QHash<olive::Track *, olive::TrackListRippleToolCommand::RippleInfo> info;
	info.insert(track, { b, true });
	olive::TrackListRippleToolCommand cmd(list, info, olive::core::rational(1),
										  olive::Timeline::kTrimOut);

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	olive::Block *gap = track->Blocks().at(1);
	EXPECT_NE(dynamic_cast<olive::GapBlock *>(gap), nullptr);
	EXPECT_EQ(gap->length(), olive::core::rational(1));
	EXPECT_EQ(gap->in(), olive::core::rational(2));
	EXPECT_EQ(gap->out(), olive::core::rational(3));
	EXPECT_EQ(b->in(), olive::core::rational(3));
	EXPECT_EQ(b->out(), olive::core::rational(6));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
	EXPECT_EQ(track->track_length(), olive::core::rational(5));
}

TEST_F(TimelineUndoTest, RippleToolCommandRemovesZeroLengthGap)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::GapBlock *g = CreateGap(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(2));
	track->AppendBlock(a);
	track->AppendBlock(g);
	track->AppendBlock(b);
	AppendTrackToList(list, track);
	// Layout: a [0,2], gap [2,4], b [4,6]

	// Trimming the gap by its entire length removes it from the track and graph
	QHash<olive::Track *, olive::TrackListRippleToolCommand::RippleInfo> info;
	info.insert(track, { g, false });
	olive::TrackListRippleToolCommand cmd(list, info,
										  olive::core::rational(-2),
										  olive::Timeline::kTrimOut);

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(4));
	EXPECT_EQ(g->track(), nullptr);
	EXPECT_EQ(g->project(), nullptr);

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(track->Blocks().at(1), g);
	EXPECT_EQ(g->project(), project_.get());
	EXPECT_EQ(g->in(), olive::core::rational(2));
	EXPECT_EQ(g->out(), olive::core::rational(4));
	EXPECT_EQ(b->in(), olive::core::rational(4));
	EXPECT_EQ(b->out(), olive::core::rational(6));
}

TEST_F(TimelineUndoTest, RippleDeleteGapsAtRegionsRemovesGap)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::GapBlock *g = CreateGap(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(2));
	track->AppendBlock(a);
	track->AppendBlock(g);
	track->AppendBlock(b);
	AppendTrackToList(sequence->track_list(olive::Track::kVideo), track);
	// Layout: a [0,2], gap [2,4], b [4,6]

	olive::TimelineRippleDeleteGapsAtRegionsCommand::RangeList regions;
	regions.append({ track, olive::core::TimeRange(olive::core::rational(2),
												   olive::core::rational(4)) });
	olive::TimelineRippleDeleteGapsAtRegionsCommand cmd(sequence, regions);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	// commands_ is populated by prepare(), which runs on the first redo
	cmd.redo_now();
	EXPECT_TRUE(cmd.HasCommands());
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(4));
	EXPECT_EQ(g->track(), nullptr);

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(track->Blocks().at(1), g);
	EXPECT_EQ(g->in(), olive::core::rational(2));
	EXPECT_EQ(g->out(), olive::core::rational(4));
	EXPECT_EQ(b->in(), olive::core::rational(4));
	EXPECT_EQ(b->out(), olive::core::rational(6));
}

TEST_F(TimelineUndoTest, RippleDeleteGapsAtRegionsIgnoresNonGapRegion)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	track->AppendBlock(a);
	AppendTrackToList(sequence->track_list(olive::Track::kVideo), track);
	// Layout: a [0,2]

	// The region covers a clip rather than a gap, so there is nothing to do
	olive::TimelineRippleDeleteGapsAtRegionsCommand::RangeList regions;
	regions.append({ track, olive::core::TimeRange(olive::core::rational(0),
												   olive::core::rational(2)) });
	olive::TimelineRippleDeleteGapsAtRegionsCommand cmd(sequence, regions);

	// prepare() finds no gap for the region, so no sub-commands are created
	cmd.redo_now();
	EXPECT_FALSE(cmd.HasCommands());
	ASSERT_EQ(track->Blocks().size(), 1);
	EXPECT_EQ(a->length(), olive::core::rational(2));

	cmd.undo_now();
	EXPECT_EQ(track->track_length(), olive::core::rational(2));
}

//
// timelineundopointer.h
//
TEST_F(TimelineUndoTest, BlockTrimCommandTrimOutCreatesGap)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	track->AppendBlock(a);
	track->AppendBlock(b);
	// Layout: a [0,2], b [2,5]

	// Trimming a shorter with a clip adjacent creates a gap to fill the space
	olive::BlockTrimCommand cmd(track, a, olive::core::rational(1),
								olive::Timeline::kTrimOut);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(a->length(), olive::core::rational(1));
	EXPECT_EQ(a->out(), olive::core::rational(1));
	olive::Block *gap = track->Blocks().at(1);
	EXPECT_NE(dynamic_cast<olive::GapBlock *>(gap), nullptr);
	EXPECT_EQ(gap->in(), olive::core::rational(1));
	EXPECT_EQ(gap->out(), olive::core::rational(2));
	// b is unaffected
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
	EXPECT_EQ(track->track_length(), olive::core::rational(5));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(a->length(), olive::core::rational(2));
	EXPECT_EQ(a->out(), olive::core::rational(2));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
}

TEST_F(TimelineUndoTest, BlockTrimCommandTrimOutIntoGap)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::GapBlock *g = CreateGap(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(2));
	track->AppendBlock(a);
	track->AppendBlock(g);
	track->AppendBlock(b);
	// Layout: a [0,2], gap [2,4], b [4,6]

	// Trimming a longer consumes time from the adjacent gap
	olive::BlockTrimCommand cmd(track, a, olive::core::rational(3),
								olive::Timeline::kTrimOut);

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(a->length(), olive::core::rational(3));
	EXPECT_EQ(a->out(), olive::core::rational(3));
	EXPECT_EQ(g->length(), olive::core::rational(1));
	EXPECT_EQ(g->in(), olive::core::rational(3));
	EXPECT_EQ(g->out(), olive::core::rational(4));
	EXPECT_EQ(b->in(), olive::core::rational(4));
	EXPECT_EQ(b->out(), olive::core::rational(6));

	cmd.undo_now();
	EXPECT_EQ(a->length(), olive::core::rational(2));
	EXPECT_EQ(a->out(), olive::core::rational(2));
	EXPECT_EQ(g->length(), olive::core::rational(2));
	EXPECT_EQ(g->in(), olive::core::rational(2));
	EXPECT_EQ(g->out(), olive::core::rational(4));
	EXPECT_EQ(b->in(), olive::core::rational(4));
}

TEST_F(TimelineUndoTest, BlockTrimCommandTrimOutConsumesWholeGap)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::GapBlock *g = CreateGap(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(2));
	track->AppendBlock(a);
	track->AppendBlock(g);
	track->AppendBlock(b);
	// Layout: a [0,2], gap [2,4], b [4,6]

	// Trimming a longer by the exact gap length removes the gap entirely
	olive::BlockTrimCommand cmd(track, a, olive::core::rational(4),
								olive::Timeline::kTrimOut);

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(a->length(), olive::core::rational(4));
	EXPECT_EQ(a->out(), olive::core::rational(4));
	EXPECT_EQ(b->in(), olive::core::rational(4));
	EXPECT_EQ(b->out(), olive::core::rational(6));
	// By default the consumed gap is removed from the graph too
	EXPECT_EQ(g->track(), nullptr);
	EXPECT_EQ(g->project(), nullptr);

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(track->Blocks().at(1), g);
	EXPECT_EQ(g->project(), project_.get());
	EXPECT_EQ(g->length(), olive::core::rational(2));
	EXPECT_EQ(g->in(), olive::core::rational(2));
	EXPECT_EQ(g->out(), olive::core::rational(4));
	EXPECT_EQ(a->length(), olive::core::rational(2));
	EXPECT_EQ(b->in(), olive::core::rational(4));
}

TEST_F(TimelineUndoTest, BlockTrimCommandConsumedGapCanStayInGraph)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::GapBlock *g = CreateGap(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(2));
	track->AppendBlock(a);
	track->AppendBlock(g);
	track->AppendBlock(b);
	// Layout: a [0,2], gap [2,4], b [4,6]

	olive::BlockTrimCommand cmd(track, a, olive::core::rational(4),
								olive::Timeline::kTrimOut);
	cmd.SetRemoveZeroLengthFromGraph(false);

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	// The gap is off the track but remains in the graph when asked to stay
	EXPECT_EQ(g->track(), nullptr);
	EXPECT_EQ(g->project(), project_.get());

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(track->Blocks().at(1), g);
	EXPECT_EQ(g->in(), olive::core::rational(2));
	EXPECT_EQ(g->out(), olive::core::rational(4));
	EXPECT_EQ(a->length(), olive::core::rational(2));
}

TEST_F(TimelineUndoTest, BlockTrimCommandTrimInCreatesGap)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	track->AppendBlock(a);
	track->AppendBlock(b);
	// Layout: a [0,2], b [2,5]

	// Trimming b's in point shorter with a clip before it creates a gap
	olive::BlockTrimCommand cmd(track, b, olive::core::rational(2),
								olive::Timeline::kTrimIn);

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(track->Blocks().at(0), a);
	olive::Block *gap = track->Blocks().at(1);
	EXPECT_NE(dynamic_cast<olive::GapBlock *>(gap), nullptr);
	EXPECT_EQ(gap->in(), olive::core::rational(2));
	EXPECT_EQ(gap->out(), olive::core::rational(3));
	EXPECT_EQ(b->length(), olive::core::rational(2));
	EXPECT_EQ(b->in(), olive::core::rational(3));
	EXPECT_EQ(b->out(), olive::core::rational(5));
	EXPECT_EQ(b->media_in(), olive::core::rational(1));
	// a is unaffected
	EXPECT_EQ(a->in(), olive::core::rational(0));
	EXPECT_EQ(a->out(), olive::core::rational(2));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(b->length(), olive::core::rational(3));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
	EXPECT_EQ(b->media_in(), olive::core::rational(0));
}

TEST_F(TimelineUndoTest, BlockTrimCommandTrimInRollEdit)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	track->AppendBlock(a);
	track->AppendBlock(b);
	// Layout: a [0,2], b [2,5]

	// A roll edit extends the adjacent clip instead of creating a gap
	olive::BlockTrimCommand cmd(track, b, olive::core::rational(2),
								olive::Timeline::kTrimIn);
	cmd.SetTrimIsARollEdit(true);

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(a->length(), olive::core::rational(3));
	EXPECT_EQ(a->out(), olive::core::rational(3));
	EXPECT_EQ(b->length(), olive::core::rational(2));
	EXPECT_EQ(b->in(), olive::core::rational(3));
	EXPECT_EQ(b->out(), olive::core::rational(5));
	EXPECT_EQ(b->media_in(), olive::core::rational(1));
	EXPECT_EQ(track->track_length(), olive::core::rational(5));

	cmd.undo_now();
	EXPECT_EQ(a->length(), olive::core::rational(2));
	EXPECT_EQ(a->out(), olive::core::rational(2));
	EXPECT_EQ(b->length(), olive::core::rational(3));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
	EXPECT_EQ(b->media_in(), olive::core::rational(0));
}

TEST_F(TimelineUndoTest, BlockTrimCommandTrimOutLastBlockHasNoAdjacent)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	track->AppendBlock(a);
	track->AppendBlock(b);
	// Layout: a [0,2], b [2,5]

	// Trimming the last block's out point shorter shortens the whole track
	olive::BlockTrimCommand cmd(track, b, olive::core::rational(1),
								olive::Timeline::kTrimOut);

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(b->length(), olive::core::rational(1));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(3));
	EXPECT_EQ(track->track_length(), olive::core::rational(3));

	cmd.undo_now();
	EXPECT_EQ(b->length(), olive::core::rational(3));
	EXPECT_EQ(b->out(), olive::core::rational(5));
	EXPECT_EQ(track->track_length(), olive::core::rational(5));
}

TEST_F(TimelineUndoTest, BlockTrimCommandSameLengthDoesNothing)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	track->AppendBlock(a);
	track->AppendBlock(b);

	// Trimming to the current length is a no-op
	olive::BlockTrimCommand cmd(track, a, olive::core::rational(2),
								olive::Timeline::kTrimOut);

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(a->length(), olive::core::rational(2));
	EXPECT_EQ(b->in(), olive::core::rational(2));

	cmd.undo_now();
	EXPECT_EQ(a->length(), olive::core::rational(2));
	EXPECT_EQ(b->in(), olive::core::rational(2));
}

TEST_F(TimelineUndoTest, TrackSlideCommandShiftsGaps)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::GapBlock *g1 = CreateGap(project_.get(), olive::core::rational(2));
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(3));
	olive::GapBlock *g2 = CreateGap(project_.get(), olive::core::rational(2));
	track->AppendBlock(g1);
	track->AppendBlock(a);
	track->AppendBlock(g2);
	// Layout: gap [0,2], a [2,5], gap [5,7]

	// Sliding a one frame later grows the leading gap and shrinks the trailing one
	olive::TrackSlideCommand cmd(track, { a }, g1, g2,
								 olive::core::rational(1));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_EQ(g1->length(), olive::core::rational(3));
	EXPECT_EQ(g1->out(), olive::core::rational(3));
	EXPECT_EQ(a->in(), olive::core::rational(3));
	EXPECT_EQ(a->out(), olive::core::rational(6));
	EXPECT_EQ(g2->length(), olive::core::rational(1));
	EXPECT_EQ(g2->in(), olive::core::rational(6));
	EXPECT_EQ(g2->out(), olive::core::rational(7));
	EXPECT_EQ(track->track_length(), olive::core::rational(7));

	cmd.undo_now();
	EXPECT_EQ(g1->length(), olive::core::rational(2));
	EXPECT_EQ(a->in(), olive::core::rational(2));
	EXPECT_EQ(a->out(), olive::core::rational(5));
	EXPECT_EQ(g2->length(), olive::core::rational(2));
	EXPECT_EQ(g2->in(), olive::core::rational(5));
	EXPECT_EQ(g2->out(), olive::core::rational(7));
}

TEST_F(TimelineUndoTest, TrackSlideCommandRemovesOutAdjacent)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::GapBlock *g1 = CreateGap(project_.get(), olive::core::rational(2));
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(3));
	olive::GapBlock *g2 = CreateGap(project_.get(), olive::core::rational(2));
	track->AppendBlock(g1);
	track->AppendBlock(a);
	track->AppendBlock(g2);
	// Layout: gap [0,2], a [2,5], gap [5,7]

	// Sliding right by the trailing gap's length consumes it
	olive::TrackSlideCommand cmd(track, { a }, g1, g2,
								 olive::core::rational(2));

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(0), g1);
	EXPECT_EQ(track->Blocks().at(1), a);
	EXPECT_EQ(g1->length(), olive::core::rational(4));
	EXPECT_EQ(g1->out(), olive::core::rational(4));
	EXPECT_EQ(a->in(), olive::core::rational(4));
	EXPECT_EQ(a->out(), olive::core::rational(7));
	EXPECT_EQ(g2->track(), nullptr);
	EXPECT_EQ(g2->project(), nullptr);
	EXPECT_EQ(track->track_length(), olive::core::rational(7));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(track->Blocks().at(0), g1);
	EXPECT_EQ(track->Blocks().at(1), a);
	EXPECT_EQ(track->Blocks().at(2), g2);
	EXPECT_EQ(g2->project(), project_.get());
	EXPECT_EQ(g1->length(), olive::core::rational(2));
	EXPECT_EQ(g1->out(), olive::core::rational(2));
	EXPECT_EQ(a->in(), olive::core::rational(2));
	EXPECT_EQ(a->out(), olive::core::rational(5));
	EXPECT_EQ(g2->in(), olive::core::rational(5));
	EXPECT_EQ(g2->out(), olive::core::rational(7));
}

TEST_F(TimelineUndoTest, TrackSlideCommandRemovesInAdjacent)
{
	olive::Track *track = CreateTrack(project_.get());
	olive::GapBlock *g1 = CreateGap(project_.get(), olive::core::rational(2));
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(3));
	olive::GapBlock *g2 = CreateGap(project_.get(), olive::core::rational(2));
	track->AppendBlock(g1);
	track->AppendBlock(a);
	track->AppendBlock(g2);
	// Layout: gap [0,2], a [2,5], gap [5,7]

	// Sliding left by the leading gap's length consumes it
	olive::TrackSlideCommand cmd(track, { a }, g1, g2,
								 olive::core::rational(-2));

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(track->Blocks().at(1), g2);
	EXPECT_EQ(a->in(), olive::core::rational(0));
	EXPECT_EQ(a->out(), olive::core::rational(3));
	EXPECT_EQ(g2->length(), olive::core::rational(4));
	EXPECT_EQ(g2->in(), olive::core::rational(3));
	EXPECT_EQ(g2->out(), olive::core::rational(7));
	EXPECT_EQ(g1->track(), nullptr);
	EXPECT_EQ(g1->project(), nullptr);

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(track->Blocks().at(0), g1);
	EXPECT_EQ(track->Blocks().at(1), a);
	EXPECT_EQ(track->Blocks().at(2), g2);
	EXPECT_EQ(g1->project(), project_.get());
	EXPECT_EQ(g1->in(), olive::core::rational(0));
	EXPECT_EQ(g1->out(), olive::core::rational(2));
	EXPECT_EQ(a->in(), olive::core::rational(2));
	EXPECT_EQ(a->out(), olive::core::rational(5));
	EXPECT_EQ(g2->length(), olive::core::rational(2));
	EXPECT_EQ(g2->in(), olive::core::rational(5));
	EXPECT_EQ(g2->out(), olive::core::rational(7));
}

TEST_F(TimelineUndoTest, TrackPlaceBlockCommandAppendsToEmptyTrack)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);
	olive::Track *track = CreateTrack(project_.get());
	AppendTrackToList(list, track);

	olive::ClipBlock *p = CreateClip(project_.get(), olive::core::rational(2));
	olive::TrackPlaceBlockCommand cmd(list, 0, p, olive::core::rational(0));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 1);
	EXPECT_EQ(track->Blocks().at(0), p);
	EXPECT_EQ(p->in(), olive::core::rational(0));
	EXPECT_EQ(p->out(), olive::core::rational(2));

	cmd.undo_now();
	EXPECT_TRUE(track->Blocks().isEmpty());
	EXPECT_EQ(p->track(), nullptr);
}

TEST_F(TimelineUndoTest, TrackPlaceBlockCommandInsertsGapToReachPoint)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);
	olive::Track *track = CreateTrack(project_.get());
	AppendTrackToList(list, track);

	// Placing past the end of the track pads with a gap
	olive::ClipBlock *p = CreateClip(project_.get(), olive::core::rational(2));
	olive::TrackPlaceBlockCommand cmd(list, 0, p, olive::core::rational(3));

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 2);
	olive::Block *gap = track->Blocks().at(0);
	EXPECT_NE(dynamic_cast<olive::GapBlock *>(gap), nullptr);
	EXPECT_EQ(gap->in(), olive::core::rational(0));
	EXPECT_EQ(gap->out(), olive::core::rational(3));
	EXPECT_EQ(track->Blocks().at(1), p);
	EXPECT_EQ(p->in(), olive::core::rational(3));
	EXPECT_EQ(p->out(), olive::core::rational(5));
	EXPECT_EQ(track->track_length(), olive::core::rational(5));

	cmd.undo_now();
	EXPECT_TRUE(track->Blocks().isEmpty());
	EXPECT_EQ(track->track_length(), olive::core::rational(0));
}

TEST_F(TimelineUndoTest, TrackPlaceBlockCommandOverwritesMiddle)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);
	olive::Track *track = CreateTrack(project_.get());
	olive::ClipBlock *a = CreateClip(project_.get(), olive::core::rational(2));
	olive::ClipBlock *b = CreateClip(project_.get(), olive::core::rational(3));
	olive::ClipBlock *c = CreateClip(project_.get(), olive::core::rational(1));
	track->AppendBlock(a);
	track->AppendBlock(b);
	track->AppendBlock(c);
	AppendTrackToList(list, track);
	// Layout: a [0,2], b [2,5], c [5,6]

	// Placing a two-second block at 2 trims b's head to make room
	olive::ClipBlock *p = CreateClip(project_.get(), olive::core::rational(2));
	olive::TrackPlaceBlockCommand cmd(list, 0, p, olive::core::rational(2));

	cmd.redo_now();
	ASSERT_EQ(track->Blocks().size(), 4);
	EXPECT_EQ(track->Blocks().at(0), a);
	EXPECT_EQ(track->Blocks().at(1), p);
	EXPECT_EQ(p->in(), olive::core::rational(2));
	EXPECT_EQ(p->out(), olive::core::rational(4));
	EXPECT_EQ(track->Blocks().at(2), b);
	EXPECT_EQ(b->length(), olive::core::rational(1));
	EXPECT_EQ(b->in(), olive::core::rational(4));
	EXPECT_EQ(b->out(), olive::core::rational(5));
	EXPECT_EQ(track->Blocks().at(3), c);
	EXPECT_EQ(c->in(), olive::core::rational(5));
	EXPECT_EQ(c->out(), olive::core::rational(6));

	cmd.undo_now();
	ASSERT_EQ(track->Blocks().size(), 3);
	EXPECT_EQ(p->track(), nullptr);
	EXPECT_EQ(track->Blocks().at(1), b);
	EXPECT_EQ(b->length(), olive::core::rational(3));
	EXPECT_EQ(b->in(), olive::core::rational(2));
	EXPECT_EQ(b->out(), olive::core::rational(5));
	EXPECT_EQ(c->in(), olive::core::rational(5));
}

TEST_F(TimelineUndoTest, TrackPlaceBlockCommandAddsMissingTracks)
{
	olive::Sequence *sequence = CreateSequence(project_.get());
	olive::TrackList *list = sequence->track_list(olive::Track::kVideo);
	ASSERT_EQ(list->GetTrackCount(), 0);

	// Placing on track index 1 of an empty list creates both tracks
	olive::ClipBlock *p = CreateClip(project_.get(), olive::core::rational(2));
	olive::TrackPlaceBlockCommand cmd(list, 1, p, olive::core::rational(0));

	cmd.redo_now();
	ASSERT_EQ(list->GetTrackCount(), 2);
	olive::Track *placed_track = list->GetTrackAt(1);
	ASSERT_NE(placed_track, nullptr);
	ASSERT_EQ(placed_track->Blocks().size(), 1);
	EXPECT_EQ(placed_track->Blocks().at(0), p);
	EXPECT_EQ(p->in(), olive::core::rational(0));
	EXPECT_EQ(p->out(), olive::core::rational(2));

	cmd.undo_now();
	EXPECT_EQ(p->track(), nullptr);
	EXPECT_EQ(list->GetTrackCount(), 0);
}
