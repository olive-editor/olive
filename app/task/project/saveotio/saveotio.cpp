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

#include "saveotio.h"

#include <opentimelineio/clip.h>
#include <opentimelineio/externalReference.h>
#include <opentimelineio/gap.h>
#include <opentimelineio/serializableCollection.h>
#include <opentimelineio/serializableObject.h>
#include <opentimelineio/transition.h>

#include "node/block/transition/transition.h"
#include "node/input/media/media.h"

OLIVE_NAMESPACE_ENTER

SaveOTIOTask::SaveOTIOTask(ProjectPtr project) :
  project_(project)
{
  SetTitle(tr("Exporting project to OpenTimelineIO"));
}

bool SaveOTIOTask::Run()
{
  QList<ItemPtr> sequences = project_->get_items_of_type(Item::kSequence);

  if (sequences.isEmpty()) {
    SetError(tr("Project contains no sequences to export."));
    return false;
  }

  std::vector<OTIO::SerializableObject*> serialized;

  foreach (ItemPtr item, sequences) {
    SequencePtr seq = std::static_pointer_cast<Sequence>(sequences.first());

    auto otio_timeline = SerializeTimeline(seq);

    if (otio_timeline) {
      // Append to list
      serialized.push_back(otio_timeline);
    } else {
      // Delete all existing timelines
      foreach (auto s, serialized) {
        s->possibly_delete();
      }

      // Error out of function
      SetError(tr("Failed to serialize sequence \"%1\"").arg(seq->name()));

      return false;
    }
  }

  OTIO::ErrorStatus es;

  if (serialized.size() == 1) {
    // Serialize timeline on its own
    auto t = serialized.front();
    t->to_json_file(project_->filename().toStdString(), &es);
    t->possibly_delete();
  } else {
    // Serialize all into a SerializableCollection
    auto collection = new OTIO::SerializableCollection("Sequences", serialized);
    collection->to_json_file(project_->filename().toStdString(), &es);
    collection->possibly_delete();

    // Delete all existing timelines
    foreach (auto s, serialized) {
      s->possibly_delete();
    }
  }

  return (es == OTIO::ErrorStatus::OK);
}

OTIO::Timeline *SaveOTIOTask::SerializeTimeline(SequencePtr sequence)
{
  auto otio_timeline = new OTIO::Timeline(sequence->name().toStdString());
  // Retainers clean themselves up when the final user is removed
  OTIO::Timeline::Retainer<OTIO::Timeline>* timeline_retainer = new OTIO::Timeline::Retainer<OTIO::Timeline>(otio_timeline);

  if (!SerializeTrackList(sequence->viewer_output()->track_list(Timeline::kTrackTypeVideo), otio_timeline)
      || !SerializeTrackList(sequence->viewer_output()->track_list(Timeline::kTrackTypeAudio), otio_timeline)) {
    otio_timeline->possibly_delete();
    return nullptr;
  }

  return otio_timeline;
}

OTIO::Track *SaveOTIOTask::SerializeTrack(TrackOutput *track)
{
  auto otio_track = new OTIO::Track();

  OTIO::ErrorStatus es;

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
    OTIO::Composable* otio_block = nullptr;

    switch (block->type()) {
    case Block::kClip:
    {
      auto otio_clip = new OTIO::Clip(block->GetLabel().toStdString());

      otio_clip->set_source_range(OTIO::TimeRange(block->in().toRationalTime(),
                                                                  block->length().toRationalTime()));

      QList<MediaInput*> media_nodes = block->FindInputNodes<MediaInput>();
      if (!media_nodes.isEmpty()) {
        auto media_ref = new OTIO::ExternalReference(media_nodes.first()->stream()->footage()->filename().toStdString());
        otio_clip->set_media_reference(media_ref);
      }

      otio_block = otio_clip;
      break;
    }
    case Block::kGap:
    {
      otio_block = new OTIO::Gap(OTIO::TimeRange(block->in().toRationalTime(),
                                 block->length().toRationalTime()),
                                 block->GetLabel().toStdString()
                                 );
      break;
    }
    case Block::kTransition:
    {
      auto otio_transition = new OTIO::Transition(block->GetLabel().toStdString());

      TransitionBlock* our_transition = static_cast<TransitionBlock*>(block);

      otio_transition->set_in_offset(our_transition->in_offset().toRationalTime());
      otio_transition->set_out_offset(our_transition->out_offset().toRationalTime());

      otio_block = new OTIO::Transition();
      break;
    }
    }

    if (!otio_block) {
      // We shouldn't ever get here, but catch without crashing if we ever do
      goto fail;
    }

    otio_track->append_child(otio_block, &es);

    if (es != OTIO::ErrorStatus::OK) {
      goto fail;
    }
  }

  return otio_track;

fail:
  otio_track->possibly_delete();

  return nullptr;
}

bool SaveOTIOTask::SerializeTrackList(TrackList *list, OTIO::Timeline* otio_timeline)
{
  OTIO::ErrorStatus es;

  foreach (TrackOutput* track, list->GetTracks()) {
    auto otio_track = SerializeTrack(track);

    if (!otio_track) {
      return false;
    }

    otio_timeline->tracks()->append_child(otio_track, &es);

    if (es != OTIO::ErrorStatus::OK) {
      otio_track->possibly_delete();
      return false;
    }
  }

  return true;
}

OLIVE_NAMESPACE_EXIT
