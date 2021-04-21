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
#include "node/math/math/math.h"
#include "node/math/merge/merge.h"
#include "node/project/project.h"
#include "node/project/sequence/sequence.h"
#include "undo/undocommand.h"
#include "widget/timelinewidget/timelineundo.h"
#include "../testutil.h"

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

    OLIVE_ASSERT(sequence.GetConnectedOutput(Sequence::kTextureInput).node() == first_video_track);
    OLIVE_ASSERT(sequence.track_list(Track::kVideo)->GetTrackCount() == 1);
    OLIVE_ASSERT(sequence.track_list(Track::kVideo)->GetTrackAt(0) == first_video_track);
  }

  {
    // Test creating initial audio track
    first_audio_track = TimelineAddTrackCommand::RunImmediately(sequence.track_list(Track::kAudio));

    OLIVE_ASSERT(sequence.GetConnectedOutput(Sequence::kSamplesInput).node() == first_audio_track);
    OLIVE_ASSERT(sequence.track_list(Track::kAudio)->GetTrackCount() == 1);
    OLIVE_ASSERT(sequence.track_list(Track::kAudio)->GetTrackAt(0) == first_audio_track);
  }

  {
    // Test creating second video track with merge
    Track* second_video_track = TimelineAddTrackCommand::RunImmediately(sequence.track_list(Track::kVideo), true);
    OLIVE_ASSERT(sequence.GetConnectedOutput(Sequence::kTextureInput).node() != first_video_track);
    OLIVE_ASSERT(sequence.GetConnectedOutput(Sequence::kTextureInput).node() != second_video_track);
    OLIVE_ASSERT(sequence.track_list(Track::kVideo)->GetTrackCount() == 2);
    OLIVE_ASSERT(sequence.track_list(Track::kVideo)->GetTrackAt(1) == second_video_track);

    MergeNode* merge = dynamic_cast<MergeNode*>(sequence.GetConnectedOutput(Sequence::kTextureInput).node());
    OLIVE_ASSERT(merge);
    OLIVE_ASSERT(merge->GetConnectedOutput(MergeNode::kBaseIn).node() == first_video_track);
    OLIVE_ASSERT(merge->GetConnectedOutput(MergeNode::kBlendIn).node() == second_video_track);
  }

  {
    // Test creating second audio track with merge
    Track* second_audio_track = TimelineAddTrackCommand::RunImmediately(sequence.track_list(Track::kAudio), true);
    OLIVE_ASSERT(sequence.GetConnectedOutput(Sequence::kSamplesInput).node() != first_audio_track);
    OLIVE_ASSERT(sequence.GetConnectedOutput(Sequence::kSamplesInput).node() != second_audio_track);
    OLIVE_ASSERT(sequence.track_list(Track::kAudio)->GetTrackCount() == 2);
    OLIVE_ASSERT(sequence.track_list(Track::kAudio)->GetTrackAt(1) == second_audio_track);

    MathNode* merge = dynamic_cast<MathNode*>(sequence.GetConnectedOutput(Sequence::kSamplesInput).node());
    OLIVE_ASSERT(merge);
    OLIVE_ASSERT(merge->GetConnectedOutput(MathNode::kParamAIn).node() == first_audio_track);
    OLIVE_ASSERT(merge->GetConnectedOutput(MathNode::kParamBIn).node() == second_audio_track);
  }

  OLIVE_TEST_END;
}

OLIVE_ADD_TEST(SequenceDefaults)
{
  TIMELINE_TEST_START;

  sequence.add_default_nodes();

  OLIVE_ASSERT(sequence.GetTracks().size() == 2);

  Track* tex_connect = dynamic_cast<Track*>(sequence.GetConnectedTextureOutput().node());
  OLIVE_ASSERT(tex_connect);
  Track* smp_connect = dynamic_cast<Track*>(sequence.GetConnectedSampleOutput().node());
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
  project.setParent(block1);
  track->AppendBlock(block1);

  ClipBlock* block2 = new ClipBlock();
  block2->set_length_and_media_out(2);
  project.setParent(block2);
  track->AppendBlock(block2);

  // There should be two blocks right now
  OLIVE_ASSERT(track->Blocks().size() == 2);

  {
    // Trim out point of second block
    BlockTrimCommand command(track, block2, 1, Timeline::kTrimOut);
    command.redo();

    // No block should have been added
    OLIVE_ASSERT(track->Blocks().size() == 2);
    OLIVE_ASSERT(block2->length() == 1);
    OLIVE_ASSERT(block1->length() == 2);

    command.undo();

    OLIVE_ASSERT(track->Blocks().size() == 2);
    OLIVE_ASSERT(block2->length() == 2);
    OLIVE_ASSERT(block1->length() == 2);
  }

  {
    // Trim in point of second block
    BlockTrimCommand command(track, block2, 1, Timeline::kTrimIn);
    command.redo();

    // Gap should be inserted in between
    OLIVE_ASSERT(track->Blocks().size() == 3);
    GapBlock* gap = dynamic_cast<GapBlock*>(track->Blocks().at(1));
    OLIVE_ASSERT(gap);
    OLIVE_ASSERT(gap->length() == 1);
    OLIVE_ASSERT(block2->length() == 1);
    OLIVE_ASSERT(block1->length() == 2);
    OLIVE_ASSERT(block1->next() == gap);
    OLIVE_ASSERT(block2->previous() == gap);

    command.undo();

    OLIVE_ASSERT(track->Blocks().size() == 2);
    OLIVE_ASSERT(block2->length() == 2);
    OLIVE_ASSERT(block1->length() == 2);
  }

  {
    // Trim out point of first block
    BlockTrimCommand command(track, block1, 1, Timeline::kTrimOut);
    command.redo();

    // Gap should be inserted in between
    OLIVE_ASSERT(track->Blocks().size() == 3);
    GapBlock* gap = dynamic_cast<GapBlock*>(track->Blocks().at(1));
    OLIVE_ASSERT(gap);
    OLIVE_ASSERT(gap->length() == 1);
    OLIVE_ASSERT(block1->length() == 1);
    OLIVE_ASSERT(block2->length() == 2);
    OLIVE_ASSERT(block1->next() == gap);
    OLIVE_ASSERT(block2->previous() == gap);

    command.undo();

    OLIVE_ASSERT(track->Blocks().size() == 2);
    OLIVE_ASSERT(block2->length() == 2);
    OLIVE_ASSERT(block1->length() == 2);
  }

  {
    // Trim in point of first block
    BlockTrimCommand command(track, block1, 1, Timeline::kTrimIn);
    command.redo();

    // Gap should be prepended to the start
    OLIVE_ASSERT(track->Blocks().size() == 3);
    GapBlock* gap = dynamic_cast<GapBlock*>(track->Blocks().at(0));
    OLIVE_ASSERT(gap);
    OLIVE_ASSERT(gap->length() == 1);
    OLIVE_ASSERT(block1->length() == 1);
    OLIVE_ASSERT(block2->length() == 2);
    OLIVE_ASSERT(block1->next() == block2);
    OLIVE_ASSERT(block1->previous() == gap);

    command.undo();

    OLIVE_ASSERT(track->Blocks().size() == 2);
    OLIVE_ASSERT(block2->length() == 2);
    OLIVE_ASSERT(block1->length() == 2);
  }

  OLIVE_TEST_END;
}

}
