/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "projectsaveasotiotask.h"

#include <opentimelineio/clip.h>
#include <opentimelineio/gap.h>
#include <opentimelineio/timeline.h>
#include <opentimelineio/transition.h>

#include "node/block/transition/transition.h"

OLIVE_NAMESPACE_ENTER

ProjectSaveAsOTIOTask::ProjectSaveAsOTIOTask(Sequence *sequence) :
  sequence_(sequence)
{
  SetTitle(tr("Saving '%1' as OpenTimelineIO").arg(sequence->name()));
}

bool ProjectSaveAsOTIOTask::Run()
{
  auto otio_timeline = new opentimelineio::v1_0::Timeline();

  ViewerOutput* viewer = sequence_->viewer_output();

  opentimelineio::v1_0::ErrorStatus es;

  foreach (TrackOutput* track, viewer->track_list(Timeline::kTrackTypeVideo)->GetTracks()) {
    auto otio_track = new opentimelineio::v1_0::Track();

    otio_track->set_kind("Video");

    foreach (Block* block, track->Blocks()) {
      opentimelineio::v1_0::Composable* otio_block = nullptr;

      switch (block->type()) {
      case Block::kClip:
      {
        auto otio_clip = new opentimelineio::v1_0::Clip();

        otio_clip->set_source_range(opentimelineio::v1_0::TimeRange(block->in().toRationalTime(),
                                                                    block->length().toRationalTime()));

        otio_block = otio_clip;
        break;
      }
      case Block::kGap:
      {
        auto otio_gap = new opentimelineio::v1_0::Gap();

        otio_gap->set_source_range(opentimelineio::v1_0::TimeRange(block->in().toRationalTime(),
                                                                   block->length().toRationalTime()));

        otio_block = otio_gap;
        break;
      }
      case Block::kTransition:
      {
        auto otio_transition = new opentimelineio::v1_0::Transition();

        TransitionBlock* our_transition = static_cast<TransitionBlock*>(block);

        otio_transition->set_in_offset(our_transition->in_offset().toRationalTime());
        otio_transition->set_out_offset(our_transition->out_offset().toRationalTime());

        otio_block = new opentimelineio::v1_0::Transition();
        break;
      }
      }

      if (!otio_block) {
        // We shouldn't ever get here, but catch without crashing if we ever do
        return false;
      }

      otio_track->append_child(otio_block, &es);

      if (es != opentimelineio::v1_0::ErrorStatus::OK) {
        return false;
      }
    }

    otio_timeline->tracks()->append_child(otio_track, &es);

    if (es != opentimelineio::v1_0::ErrorStatus::OK) {
      return false;
    }
  }

  return true;
}

OLIVE_NAMESPACE_EXIT
