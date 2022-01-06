/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "core.h"
#include "node/block/clip/clip.h"
#include "node/block/transition/crossdissolve/crossdissolvetransition.h"
#include "node/math/math/math.h"
#include "node/math/merge/merge.h"
#include "node/project/project.h"
#include "node/project/sequence/sequence.h"
#include "undo/undocommand.h"
#include "widget/timelinewidget/undo/timelineundogeneral.h"
#include "widget/timelinewidget/undo/timelineundopointer.h"
#include "testutil.h"

namespace olive {

#define TIMELINE_TEST_START \
  Project project; \
  Sequence sequence; \
  sequence.setParent(&project)

OLIVE_ADD_TEST(AddTrack)
{
  TIMELINE_TEST_START;

  Track *first_video_track, *first_audio_track;

  {
    // Test creating initial video track
    first_video_track = TimelineAddTrackCommand::RunImmediately(sequence.track_list(Track::kVideo));

    OLIVE_ASSERT(sequence.GetConnectedOutput(Sequence::kTextureInput) == first_video_track);
    OLIVE_ASSERT(sequence.track_list(Track::kVideo)->GetTrackCount() == 1);
    OLIVE_ASSERT(sequence.track_list(Track::kVideo)->GetTrackAt(0) == first_video_track);
  }

  {
    // Test creating initial audio track
    first_audio_track = TimelineAddTrackCommand::RunImmediately(sequence.track_list(Track::kAudio));

    OLIVE_ASSERT(sequence.GetConnectedOutput(Sequence::kSamplesInput) == first_audio_track);
    OLIVE_ASSERT(sequence.track_list(Track::kAudio)->GetTrackCount() == 1);
    OLIVE_ASSERT(sequence.track_list(Track::kAudio)->GetTrackAt(0) == first_audio_track);
  }

  {
    // Test creating second video track with merge
    Track* second_video_track = TimelineAddTrackCommand::RunImmediately(sequence.track_list(Track::kVideo), true);
    OLIVE_ASSERT(sequence.GetConnectedOutput(Sequence::kTextureInput) != first_video_track);
    OLIVE_ASSERT(sequence.GetConnectedOutput(Sequence::kTextureInput) != second_video_track);
    OLIVE_ASSERT(sequence.track_list(Track::kVideo)->GetTrackCount() == 2);
    OLIVE_ASSERT(sequence.track_list(Track::kVideo)->GetTrackAt(1) == second_video_track);

    MergeNode* merge = dynamic_cast<MergeNode*>(sequence.GetConnectedOutput(Sequence::kTextureInput));
    OLIVE_ASSERT(merge);
    OLIVE_ASSERT(merge->GetConnectedOutput(MergeNode::kBaseIn) == first_video_track);
    OLIVE_ASSERT(merge->GetConnectedOutput(MergeNode::kBlendIn) == second_video_track);
  }

  {
    // Test creating second audio track with merge
    Track* second_audio_track = TimelineAddTrackCommand::RunImmediately(sequence.track_list(Track::kAudio), true);
    OLIVE_ASSERT(sequence.GetConnectedOutput(Sequence::kSamplesInput) != first_audio_track);
    OLIVE_ASSERT(sequence.GetConnectedOutput(Sequence::kSamplesInput) != second_audio_track);
    OLIVE_ASSERT(sequence.track_list(Track::kAudio)->GetTrackCount() == 2);
    OLIVE_ASSERT(sequence.track_list(Track::kAudio)->GetTrackAt(1) == second_audio_track);

    MathNode* merge = dynamic_cast<MathNode*>(sequence.GetConnectedOutput(Sequence::kSamplesInput));
    OLIVE_ASSERT(merge);
    OLIVE_ASSERT(merge->GetConnectedOutput(MathNode::kParamAIn) == first_audio_track);
    OLIVE_ASSERT(merge->GetConnectedOutput(MathNode::kParamBIn) == second_audio_track);
  }

  OLIVE_TEST_END;
}

OLIVE_ADD_TEST(SequenceDefaults)
{
  TIMELINE_TEST_START;

  sequence.add_default_nodes();

  OLIVE_ASSERT(sequence.GetTracks().size() == 2);

  Track* tex_connect = dynamic_cast<Track*>(sequence.GetConnectedTextureOutput());
  OLIVE_ASSERT(tex_connect);
  Track* smp_connect = dynamic_cast<Track*>(sequence.GetConnectedSampleOutput());
  OLIVE_ASSERT(smp_connect);
  OLIVE_ASSERT(tex_connect != smp_connect);
  OLIVE_ASSERT(sequence.GetTracks().contains(tex_connect));
  OLIVE_ASSERT(sequence.GetTracks().contains(smp_connect));

  OLIVE_TEST_END;
}

OLIVE_ADD_TEST(Trim)
{
  TIMELINE_TEST_START;

  sequence.add_default_nodes();

  Track* track = sequence.GetTracks().first();

  ClipBlock* block1 = new ClipBlock();
  block1->set_length_and_media_out(2);
  block1->setParent(&project);
  track->AppendBlock(block1);

  ClipBlock* block2 = new ClipBlock();
  block2->set_length_and_media_out(2);
  block2->setParent(&project);
  track->AppendBlock(block2);

  // There should be two blocks right now
  OLIVE_ASSERT(track->Blocks().size() == 2);

  {
    // Trim out point of second block
    BlockTrimCommand command(track, block2, 1, Timeline::kTrimOut);
    command.redo_now();

    // No block should have been added
    OLIVE_ASSERT(track->Blocks().size() == 2);
    OLIVE_ASSERT(block2->length() == 1);
    OLIVE_ASSERT(block1->length() == 2);

    command.undo_now();

    OLIVE_ASSERT(track->Blocks().size() == 2);
    OLIVE_ASSERT(block2->length() == 2);
    OLIVE_ASSERT(block1->length() == 2);
  }

  {
    // Trim in point of second block
    BlockTrimCommand command(track, block2, 1, Timeline::kTrimIn);
    command.redo_now();

    // Gap should be inserted in between
    OLIVE_ASSERT(track->Blocks().size() == 3);
    GapBlock* gap = dynamic_cast<GapBlock*>(track->Blocks().at(1));
    OLIVE_ASSERT(gap);
    OLIVE_ASSERT(gap->length() == 1);
    OLIVE_ASSERT(block2->length() == 1);
    OLIVE_ASSERT(block1->length() == 2);
    OLIVE_ASSERT(block1->next() == gap);
    OLIVE_ASSERT(block2->previous() == gap);

    command.undo_now();

    OLIVE_ASSERT(track->Blocks().size() == 2);
    OLIVE_ASSERT(block2->length() == 2);
    OLIVE_ASSERT(block1->length() == 2);
  }

  {
    // Trim out point of first block
    BlockTrimCommand command(track, block1, 1, Timeline::kTrimOut);
    command.redo_now();

    // Gap should be inserted in between
    OLIVE_ASSERT(track->Blocks().size() == 3);
    GapBlock* gap = dynamic_cast<GapBlock*>(track->Blocks().at(1));
    OLIVE_ASSERT(gap);
    OLIVE_ASSERT(gap->length() == 1);
    OLIVE_ASSERT(block1->length() == 1);
    OLIVE_ASSERT(block2->length() == 2);
    OLIVE_ASSERT(block1->next() == gap);
    OLIVE_ASSERT(block2->previous() == gap);

    command.undo_now();

    OLIVE_ASSERT(track->Blocks().size() == 2);
    OLIVE_ASSERT(block2->length() == 2);
    OLIVE_ASSERT(block1->length() == 2);
  }

  {
    // Trim in point of first block
    BlockTrimCommand command(track, block1, 1, Timeline::kTrimIn);
    command.redo_now();

    // Gap should be prepended to the start
    OLIVE_ASSERT(track->Blocks().size() == 3);
    GapBlock* gap = dynamic_cast<GapBlock*>(track->Blocks().at(0));
    OLIVE_ASSERT(gap);
    OLIVE_ASSERT(gap->length() == 1);
    OLIVE_ASSERT(block1->length() == 1);
    OLIVE_ASSERT(block2->length() == 2);
    OLIVE_ASSERT(block1->next() == block2);
    OLIVE_ASSERT(block1->previous() == gap);

    command.undo_now();

    OLIVE_ASSERT(track->Blocks().size() == 2);
    OLIVE_ASSERT(block2->length() == 2);
    OLIVE_ASSERT(block1->length() == 2);
  }

  OLIVE_TEST_END;
}

OLIVE_ADD_TEST(ReplaceBlockWithGap_ClipsOnly)
{
  TIMELINE_TEST_START;

  // Create a track that goes clip -> clip -> clip
  sequence.add_default_nodes();
  Track* track = sequence.track_list(Track::kVideo)->GetTracks().first();

  ClipBlock* a = new ClipBlock();
  a->setParent(&project);
  track->AppendBlock(a);

  ClipBlock* b = new ClipBlock();
  b->setParent(&project);
  track->AppendBlock(b);

  ClipBlock* c = new ClipBlock();
  c->setParent(&project);
  track->AppendBlock(c);

  {
    // Replace clip C with a gap
    TrackReplaceBlockWithGapCommand command(track, c);
    command.redo_now();

    // Clip should be removed without any gap actually taking its place, since the clip is at the
    // end of the track
    OLIVE_ASSERT(track->Blocks().size() == 2);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(track->Blocks().at(1) == b);

    command.undo_now();

    OLIVE_ASSERT(track->Blocks().size() == 3);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(track->Blocks().at(1) == b);
    OLIVE_ASSERT(track->Blocks().at(2) == c);
  }

  {
    // Replace clip B with a gap
    TrackReplaceBlockWithGapCommand command(track, b);
    command.redo_now();

    // B should be replaced with a gap
    OLIVE_ASSERT(track->Blocks().size() == 3);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(track->Blocks().at(1) != b);
    OLIVE_ASSERT(dynamic_cast<GapBlock*>(track->Blocks().at(1)));
    OLIVE_ASSERT(track->Blocks().at(1)->length() == b->length());
    OLIVE_ASSERT(track->Blocks().at(2) == c);

    command.undo_now();

    OLIVE_ASSERT(track->Blocks().size() == 3);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(track->Blocks().at(1) == b);
    OLIVE_ASSERT(track->Blocks().at(2) == c);
  }

  OLIVE_TEST_END;
}

OLIVE_ADD_TEST(ReplaceBlockWithGap_ClipsAndGaps)
{
  TIMELINE_TEST_START;

  // Create a track that goes clip -> gap -> clip -> clip -> gap -> clip
  sequence.add_default_nodes();
  Track* track = sequence.track_list(Track::kVideo)->GetTracks().first();

  ClipBlock* a = new ClipBlock();
  a->setParent(&project);
  track->AppendBlock(a);

  GapBlock* b = new GapBlock();
  b->setParent(&project);
  track->AppendBlock(b);

  ClipBlock* c = new ClipBlock();
  c->setParent(&project);
  track->AppendBlock(c);

  GapBlock* d = new GapBlock();
  d->setParent(&project);
  track->AppendBlock(d);

  ClipBlock* e = new ClipBlock();
  e->setParent(&project);
  track->AppendBlock(e);

  {
    // Replace clip E with a gap
    TrackReplaceBlockWithGapCommand command(track, e);
    command.redo_now();

    // Both clips D and E should be removed because this command should remove any trailing gaps
    OLIVE_ASSERT(track->Blocks().size() == 3);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(track->Blocks().at(1) == b);
    OLIVE_ASSERT(track->Blocks().at(2) == c);

    // Test undo
    command.undo_now();

    OLIVE_ASSERT(track->Blocks().size() == 5);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(track->Blocks().at(1) == b);
    OLIVE_ASSERT(track->Blocks().at(2) == c);
    OLIVE_ASSERT(track->Blocks().at(3) == d);
    OLIVE_ASSERT(track->Blocks().at(4) == e);
  }

  {
    // Replace clip A with a gap
    rational original_length_of_a = a->length();
    rational original_length_of_b = b->length();

    TrackReplaceBlockWithGapCommand command(track, a);
    command.redo_now();

    // A should be removed and B should take its place
    OLIVE_ASSERT(track->Blocks().size() == 4);

    OLIVE_ASSERT(track->Blocks().at(0) == b);
    OLIVE_ASSERT(track->Blocks().at(1) == c);
    OLIVE_ASSERT(track->Blocks().at(2) == d);
    OLIVE_ASSERT(track->Blocks().at(3) == e);
    OLIVE_ASSERT(b->length() == original_length_of_a + original_length_of_b);

    // Test undo
    command.undo_now();

    OLIVE_ASSERT(track->Blocks().size() == 5);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(track->Blocks().at(1) == b);
    OLIVE_ASSERT(track->Blocks().at(2) == c);
    OLIVE_ASSERT(track->Blocks().at(3) == d);
    OLIVE_ASSERT(track->Blocks().at(4) == e);
    OLIVE_ASSERT(a->length() == original_length_of_a);
    OLIVE_ASSERT(b->length() == original_length_of_b);
  }

  {
    // Replace clip C with a gap
    rational original_length_of_b = b->length();
    rational original_length_of_c = c->length();
    rational original_length_of_d = d->length();

    TrackReplaceBlockWithGapCommand command(track, c);
    command.redo_now();

    // C and D should be removed, and B should take both of their places
    OLIVE_ASSERT(track->Blocks().size() == 3);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(track->Blocks().at(1) == b);
    OLIVE_ASSERT(track->Blocks().at(2) == e);
    OLIVE_ASSERT(b->length() == original_length_of_b + original_length_of_c + original_length_of_d);

    // Test undo
    command.undo_now();

    OLIVE_ASSERT(track->Blocks().size() == 5);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(track->Blocks().at(1) == b);
    OLIVE_ASSERT(track->Blocks().at(2) == c);
    OLIVE_ASSERT(track->Blocks().at(3) == d);
    OLIVE_ASSERT(track->Blocks().at(4) == e);
    OLIVE_ASSERT(b->length() == original_length_of_b);
    OLIVE_ASSERT(c->length() == original_length_of_c);
    OLIVE_ASSERT(d->length() == original_length_of_d);
  }

  {
    // Add a fourth clip at the end of the track
    ClipBlock* f = new ClipBlock();
    f->setParent(&project);
    track->AppendBlock(f);

    // Try replacing E with a block again
    TrackReplaceBlockWithGapCommand command(track, e);
    rational original_length_of_d = d->length();
    rational original_length_of_e = e->length();
    command.redo_now();

    // E should be removed and D should have taken its place
    OLIVE_ASSERT(track->Blocks().size() == 5);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(track->Blocks().at(1) == b);
    OLIVE_ASSERT(track->Blocks().at(2) == c);
    OLIVE_ASSERT(track->Blocks().at(3) == d);
    OLIVE_ASSERT(track->Blocks().at(4) == f);
    OLIVE_ASSERT(d->length() == original_length_of_d + original_length_of_e);

    command.undo_now();

    OLIVE_ASSERT(track->Blocks().size() == 6);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(track->Blocks().at(1) == b);
    OLIVE_ASSERT(track->Blocks().at(2) == c);
    OLIVE_ASSERT(track->Blocks().at(3) == d);
    OLIVE_ASSERT(track->Blocks().at(4) == e);
    OLIVE_ASSERT(track->Blocks().at(5) == f);
    OLIVE_ASSERT(d->length() == original_length_of_d);
    OLIVE_ASSERT(e->length() == original_length_of_e);
  }

  OLIVE_TEST_END;
}

#define UsingTransition CrossDissolveTransition

OLIVE_ADD_TEST(ReplaceBlockWithGap_ClipsAndTransitions)
{
  TIMELINE_TEST_START;

  // Create a track that goes clip -> gap -> clip -> clip -> gap -> clip
  sequence.add_default_nodes();
  Track* track = sequence.track_list(Track::kVideo)->GetTracks().first();

  UsingTransition* a_in = new UsingTransition();
  a_in->setParent(&project);
  track->AppendBlock(a_in);

  ClipBlock* a = new ClipBlock();
  a->setParent(&project);
  track->AppendBlock(a);

  UsingTransition* a_to_b = new UsingTransition();
  a_to_b->setParent(&project);
  track->AppendBlock(a_to_b);

  ClipBlock* b = new ClipBlock();
  b->setParent(&project);
  track->AppendBlock(b);

  UsingTransition* b_out = new UsingTransition();
  b_out->setParent(&project);
  track->AppendBlock(b_out);

  Node::ConnectEdge(a, NodeInput(a_in, UsingTransition::kInBlockInput));
  Node::ConnectEdge(a, NodeInput(a_to_b, UsingTransition::kOutBlockInput));
  Node::ConnectEdge(b, NodeInput(a_to_b, UsingTransition::kInBlockInput));
  Node::ConnectEdge(b, NodeInput(b_out, UsingTransition::kOutBlockInput));

  {
    // Replace A with gap
    TrackReplaceBlockWithGapCommand command(track, a);
    command.redo_now();

    // A should be replaced with a gap and so should A_IN since A was the only clip connected to it.
    // Also A_TO_B should only be connected to B now
    OLIVE_ASSERT(track->Blocks().size() == 4);
    OLIVE_ASSERT(dynamic_cast<GapBlock*>(track->Blocks().at(0)));
    OLIVE_ASSERT(track->Blocks().at(1) == a_to_b);
    OLIVE_ASSERT(track->Blocks().at(2) == b);
    OLIVE_ASSERT(track->Blocks().at(3) == b_out);

    command.undo_now();

    OLIVE_ASSERT(track->Blocks().size() == 5);
    OLIVE_ASSERT(track->Blocks().at(0) == a_in);
    OLIVE_ASSERT(track->Blocks().at(1) == a);
    OLIVE_ASSERT(track->Blocks().at(2) == a_to_b);
    OLIVE_ASSERT(track->Blocks().at(3) == b);
    OLIVE_ASSERT(track->Blocks().at(4) == b_out);
  }

  OLIVE_TEST_END;
}

OLIVE_ADD_TEST(InsertGaps_SingleTrack)
{
  TIMELINE_TEST_START;

  sequence.add_default_nodes();

  TrackList *list = sequence.track_list(Track::kVideo);
  Track *track = list->GetTracks().first();

  ClipBlock *a = new ClipBlock();
  a->set_length_and_media_out(1);
  a->setParent(&project);
  track->AppendBlock(a);

  ClipBlock *b = new ClipBlock();
  b->set_length_and_media_out(1);
  b->setParent(&project);
  track->AppendBlock(b);

  ClipBlock *c = new ClipBlock();
  c->set_length_and_media_out(1);
  c->setParent(&project);
  track->AppendBlock(c);

  OLIVE_ASSERT(track->Blocks().size() == 3);
  OLIVE_ASSERT(track->Blocks().at(0) == a);
  OLIVE_ASSERT(track->Blocks().at(1) == b);
  OLIVE_ASSERT(track->Blocks().at(2) == c);

  {
    // Insert gap at the start of the track, all blocks should be unsplit and shifted to the right
    TrackListInsertGaps command(list, 0, 2);
    command.redo_now();

    OLIVE_ASSERT(track->Blocks().size() == 4);
    OLIVE_ASSERT(dynamic_cast<GapBlock *>(track->Blocks().at(0)));
    OLIVE_ASSERT(track->Blocks().at(0)->length() == 2);
    OLIVE_ASSERT(track->Blocks().at(1) == a);
    OLIVE_ASSERT(track->Blocks().at(2) == b);
    OLIVE_ASSERT(track->Blocks().at(3) == c);

    command.undo_now();

    OLIVE_ASSERT(track->Blocks().size() == 3);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(track->Blocks().at(1) == b);
    OLIVE_ASSERT(track->Blocks().at(2) == c);
  }

  {
    // Insert gap in the middle of block A, block A should be halved with a copy at 2 and the gap at 1
    TrackListInsertGaps command(list, rational(1, 2), 2);
    command.redo_now();

    OLIVE_ASSERT(track->Blocks().size() == 5);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(track->Blocks().at(0)->length() == rational(1, 2));
    OLIVE_ASSERT(dynamic_cast<GapBlock *>(track->Blocks().at(1)));
    OLIVE_ASSERT(dynamic_cast<ClipBlock*>(track->Blocks().at(2)));
    OLIVE_ASSERT(track->Blocks().at(3) == b);
    OLIVE_ASSERT(track->Blocks().at(4) == c);

    command.undo_now();

    OLIVE_ASSERT_EQUAL(track->Blocks().size(), 3);
    OLIVE_ASSERT_EQUAL(track->Blocks().at(0), a);
    OLIVE_ASSERT_EQUAL(track->Blocks().at(0)->length(), 1);
    OLIVE_ASSERT_EQUAL(track->Blocks().at(1), b);
    OLIVE_ASSERT_EQUAL(track->Blocks().at(2), c);
  }

  {
    // Insert gap between block A and B, blocks should be unsplit with a gap at 1
    TrackListInsertGaps command(list, 1, 2);
    command.redo_now();

    OLIVE_ASSERT(track->Blocks().size() == 4);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(dynamic_cast<GapBlock *>(track->Blocks().at(1)));
    OLIVE_ASSERT(track->Blocks().at(2) == b);
    OLIVE_ASSERT(track->Blocks().at(3) == c);

    command.undo_now();

    OLIVE_ASSERT(track->Blocks().size() == 3);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(track->Blocks().at(1) == b);
    OLIVE_ASSERT(track->Blocks().at(2) == c);
  }

  {
    // Insert gap at end, nothing should be added
    TrackListInsertGaps command(list, 3, 2);
    command.redo_now();

    OLIVE_ASSERT(track->Blocks().size() == 3);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(track->Blocks().at(1) == b);
    OLIVE_ASSERT(track->Blocks().at(2) == c);

    command.undo_now();

    OLIVE_ASSERT(track->Blocks().size() == 3);
    OLIVE_ASSERT(track->Blocks().at(0) == a);
    OLIVE_ASSERT(track->Blocks().at(1) == b);
    OLIVE_ASSERT(track->Blocks().at(2) == c);
  }

  OLIVE_TEST_END;
}

}
