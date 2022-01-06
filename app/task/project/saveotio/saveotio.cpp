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

#include "saveotio.h"

#ifdef USE_OTIO

#include <opentimelineio/clip.h>
#include <opentimelineio/externalReference.h>
#include <opentimelineio/gap.h>
#include <opentimelineio/serializableCollection.h>
#include <opentimelineio/serializableObject.h>
#include <opentimelineio/transition.h>

#include "node/block/clip/clip.h"
#include "node/block/gap/gap.h"
#include "node/block/transition/transition.h"
#include "node/project/footage/footage.h"

namespace olive {

SaveOTIOTask::SaveOTIOTask(Project *project) :
  project_(project)
{
  SetTitle(tr("Exporting project to OpenTimelineIO"));
}

bool SaveOTIOTask::Run()
{
  QVector<Sequence*> sequences = project_->root()->ListChildrenOfType<Sequence>();

  if (sequences.isEmpty()) {
    SetError(tr("Project contains no sequences to export."));
    return false;
  }

  std::vector<OTIO::SerializableObject*> serialized;

  foreach (Sequence* seq, sequences) {
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
      SetError(tr("Failed to serialize sequence \"%1\"").arg(seq->GetLabel()));

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

  return (es.outcome == OTIO::ErrorStatus::Outcome::OK);
}

OTIO::Timeline *SaveOTIOTask::SerializeTimeline(Sequence *sequence)
{
  auto otio_timeline = new OTIO::Timeline(sequence->GetLabel().toStdString());
  // Retainers clean themselves up when the final user is removed
  OTIO::Timeline::Retainer<OTIO::Timeline>* timeline_retainer = new OTIO::Timeline::Retainer<OTIO::Timeline>(otio_timeline);
  // Suppress unused variable warning
  Q_UNUSED(timeline_retainer);

  double rate = sequence->GetVideoParams().frame_rate().toDouble();
  if (qIsNaN(rate)) {
    return nullptr;
  }

  if (!SerializeTrackList(sequence->track_list(Track::kVideo), otio_timeline, rate)
      || !SerializeTrackList(sequence->track_list(Track::kAudio), otio_timeline, rate)) {
    otio_timeline->possibly_delete();
    return nullptr;
  }

  return otio_timeline;
}

OTIO::Track *SaveOTIOTask::SerializeTrack(Track *track, double sequence_rate, rational max_track_length)
{
  auto otio_track = new OTIO::Track();

  OTIO::ErrorStatus es;

  switch (track->type()) {
  case Track::kVideo:
    otio_track->set_kind("Video");
    break;
  case Track::kAudio:
    otio_track->set_kind("Audio");
    break;
  default:
    qWarning() << "Don't know OTIO track kind for native type" << track->type();
    goto fail;
  }

  foreach (Block* block, track->Blocks()) {
    OTIO::Composable* otio_block = nullptr;

    if (dynamic_cast<ClipBlock*>(block)) {
      auto otio_clip = new OTIO::Clip(block->GetLabel().toStdString());

      otio_clip->set_source_range(OTIO::TimeRange(block->in().toRationalTime(sequence_rate),
                                                  block->length().toRationalTime(sequence_rate)));

      QVector<Footage*> media_nodes = block->FindInputNodes<Footage>();
      if (!media_nodes.isEmpty()) {

        OTIO::TimeRange available_range;
        if (otio_track->kind().compare("Video") == 0) {
          // OTIO ExternalReference uses the source clips frame rate (or sample rate) as opposed to
          // the sequences rate
          double source_frame_rate = static_cast<ClipBlock*>(block)->connected_viewer()->GetVideoParams().frame_rate().toDouble();
          available_range = OTIO::TimeRange(OTIO::RationalTime(0, source_frame_rate),
                                            OTIO::RationalTime(media_nodes.first()->GetVideoParams().duration(),
                                                               source_frame_rate));
        } else if (otio_track->kind().compare("Audio") == 0) {
          available_range = OTIO::TimeRange(OTIO::RationalTime(0, media_nodes.first()->GetAudioParams().sample_rate()),
                                  OTIO::RationalTime(media_nodes.first()->GetAudioParams().duration(),
                                                     media_nodes.first()->GetAudioParams().sample_rate()));
        }
        auto media_ref = new OTIO::ExternalReference(media_nodes.first()->filename().toStdString(), available_range);
        otio_clip->set_media_reference(media_ref);
      }

      otio_block = otio_clip;
    } else if (dynamic_cast<GapBlock*>(block)) {
      otio_block = new OTIO::Gap(OTIO::TimeRange(block->in().toRationalTime(),
                                 block->length().toRationalTime()),
                                 block->GetLabel().toStdString()
                                 );
    } else if (dynamic_cast<TransitionBlock*>(block)) {
      auto otio_transition = new OTIO::Transition(block->GetLabel().toStdString());

      TransitionBlock* our_transition = static_cast<TransitionBlock*>(block);

      otio_transition->set_in_offset(our_transition->in_offset().toRationalTime());
      otio_transition->set_out_offset(our_transition->out_offset().toRationalTime());

      otio_block = new OTIO::Transition();
    }

    if (!otio_block) {
      // We shouldn't ever get here, but catch without crashing if we ever do
      goto fail;
    }

    otio_track->append_child(otio_block, &es);

    if (es.outcome != OTIO::ErrorStatus::Outcome::OK) {
      goto fail;
    }
  }

  // All OTIO tracks must have the same duration so we add a Gap to fill the remaining time
  if (otio_track->duration(&es).to_seconds() < max_track_length.toDouble()) {
    double time_left = max_track_length.toDouble() - otio_track->duration(&es).to_seconds();

    OTIO::Gap* gap = new OTIO::Gap(OTIO::TimeRange(otio_track->duration(&es),
                                   OTIO::RationalTime(time_left, 1.0)));
    otio_track->append_child(gap, &es);

    if (es.outcome != OTIO::ErrorStatus::Outcome::OK) {
      goto fail;
    }
  }

  return otio_track;

fail:
  otio_track->possibly_delete();

  return nullptr;
}

bool SaveOTIOTask::SerializeTrackList(TrackList *list, OTIO::Timeline* otio_timeline, double sequence_rate)
{
  OTIO::ErrorStatus es;

  rational max_track_length = RATIONAL_MIN;

  foreach (Track* track, list->GetTracks()) {
    if (track->track_length() > max_track_length) {
      max_track_length = track->track_length();
    }
  }

  foreach (Track* track, list->GetTracks()) {
    auto otio_track = SerializeTrack(track, sequence_rate, max_track_length);

    if (!otio_track) {
      return false;
    }

    otio_timeline->tracks()->append_child(otio_track, &es);

    if (es.outcome != OTIO::ErrorStatus::Outcome::OK) {
      otio_track->possibly_delete();
      return false;
    }
  }

  return true;
}

}

#endif // USE_OTIO
