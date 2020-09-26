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

#include "exportotiotask.h"

#include <opentimelineio/clip.h>
#include <opentimelineio/externalReference.h>
#include <opentimelineio/gap.h>
#include <opentimelineio/transition.h>

#include "node/block/transition/transition.h"
#include "node/input/media/media.h"

OLIVE_NAMESPACE_ENTER

ExportOTIOTask::ExportOTIOTask(ViewerOutput *sequence, const QString &filename) :
  sequence_(sequence),
  filename_(filename)
{
  SetTitle(tr("Saving '%1' as OpenTimelineIO").arg(sequence_->media_name()));
}

bool ExportOTIOTask::Run()
{
  auto otio_timeline = new opentimelineio::v1_0::Timeline();

  opentimelineio::v1_0::ErrorStatus es;

  if (!SerializeTrackList(sequence_->track_list(Timeline::kTrackTypeVideo), otio_timeline)
      || !SerializeTrackList(sequence_->track_list(Timeline::kTrackTypeAudio), otio_timeline)) {
    return false;
  }

  otio_timeline->to_json_file(filename_.toStdString(), &es);

  return (es == opentimelineio::v1_0::ErrorStatus::OK);
}

opentimelineio::v1_0::Track *ExportOTIOTask::SerializeTrack(TrackOutput *track)
{
  auto otio_track = new opentimelineio::v1_0::Track();

  opentimelineio::v1_0::ErrorStatus es;

  switch (track->track_type()) {
  case Timeline::kTrackTypeVideo:
    otio_track->set_kind("Video");
    break;
  case Timeline::kTrackTypeAudio:
    otio_track->set_kind("Audio");
    break;
  default:
    qWarning() << "Don't know OTIO track kind for native type" << track->track_type();
    goto fail;
  }

  foreach (Block* block, track->Blocks()) {
    opentimelineio::v1_0::Composable* otio_block = nullptr;

    switch (block->type()) {
    case Block::kClip:
    {
      auto otio_clip = new opentimelineio::v1_0::Clip();

      otio_clip->set_source_range(opentimelineio::v1_0::TimeRange(block->in().toRationalTime(),
                                                                  block->length().toRationalTime()));

      QList<MediaInput*> media_nodes = block->FindInputNodes<MediaInput>();
      if (!media_nodes.isEmpty()) {
        auto media_ref = new opentimelineio::v1_0::ExternalReference(media_nodes.first()->footage()->footage()->filename().toStdString());
        otio_clip->set_media_reference(media_ref);
      }

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
      goto fail;
    }

    otio_track->append_child(otio_block, &es);

    if (es != opentimelineio::v1_0::ErrorStatus::OK) {
      goto fail;
    }
  }

  return otio_track;

fail:
  otio_track->possibly_delete();

  return nullptr;
}

bool ExportOTIOTask::SerializeTrackList(TrackList *list, opentimelineio::v1_0::Timeline* otio_timeline)
{
  opentimelineio::v1_0::ErrorStatus es;

  foreach (TrackOutput* track, list->GetTracks()) {
    auto otio_track = SerializeTrack(track);

    if (!otio_track) {
      return false;
    }

    otio_timeline->tracks()->append_child(otio_track, &es);

    if (es != opentimelineio::v1_0::ErrorStatus::OK) {
      return false;
    }
  }

  return true;
}

OLIVE_NAMESPACE_EXIT
