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

#include "otiodecoder.h"

#include <opentimelineio/clip.h>
#include <opentimelineio/externalReference.h>
#include <opentimelineio/timeline.h>

#include "node/block/clip/clip.h"
#include "node/block/gap/gap.h"
#include "project/item/sequence/sequence.h"

OLIVE_NAMESPACE_ENTER

OTIODecoder::OTIODecoder()
{

}

QString OTIODecoder::id()
{
  return QStringLiteral("otio");
}

ItemPtr OTIODecoder::Probe(const QString& filename, const QAtomicInt* cancelled) const
{
  if (filename.endsWith(QStringLiteral(".otio"), Qt::CaseInsensitive)) {
    opentimelineio::v1_0::ErrorStatus es;

    auto timeline = static_cast<opentimelineio::v1_0::Timeline*>(opentimelineio::v1_0::SerializableObjectWithMetadata::from_json_file(filename.toStdString(), &es));

    if (es != opentimelineio::v1_0::ErrorStatus::OK) {
      return nullptr;
    }

    SequencePtr sequence = std::make_shared<Sequence>();

    // FIXME: As far as I know, OTIO doesn't store video/audio parameters?
    sequence->set_default_parameters();

    sequence->set_name(QString::fromStdString(timeline->name()));

    for (auto c : timeline->tracks()->children()) {
      auto otio_track = static_cast<opentimelineio::v1_0::Track*>(c.value);

      // Create a new track
      TrackOutput* track = nullptr;

      // Determine what kind of track it is
      if (otio_track->kind() == "Video") {
        track = sequence->viewer_output()->track_list(Timeline::kTrackTypeVideo)->AddTrack();
      } else if (otio_track->kind() == "Audio") {
        track = sequence->viewer_output()->track_list(Timeline::kTrackTypeAudio)->AddTrack();
      } else {
        qWarning() << "Found unknown track type:" << otio_track->kind().c_str();
        continue;
      }

      // Get clips from track
      std::map<opentimelineio::v1_0::Composable*, opentimelineio::v1_0::TimeRange> clip_map = otio_track->range_of_all_children(&es);
      if (es != opentimelineio::v1_0::ErrorStatus::OK) {
        return nullptr;
      }

      for (auto it=clip_map.cbegin(); it!=clip_map.cend(); it++) {

        Block* block = nullptr;

        if (it->first->schema_name() == "Clip") {

          block = new ClipBlock();

          auto otio_clip = static_cast<opentimelineio::v1_0::Clip*>(it->first);

          /*
          if (otio_clip->media_reference()->schema_name() == "ExternalReference") {
            QString footage_url = QString::fromStdString(static_cast<opentimelineio::v1_0::ExternalReference*>(otio_clip->media_reference())->target_url());
            ItemPtr footage = Decoder::ProbeMedia(footage_url, cancelled);

          }
          */

        } else if (it->first->schema_name() == "Gap") {

          block = new GapBlock();

        } else {
          qWarning() << "Found unknown block type:" << it->first->schema_name().c_str();
        }

        block->SetLabel(QString::fromStdString(it->first->name()));
        block->set_length_and_media_out(rational::fromDouble(it->second.duration().to_seconds()));
        sequence->AddNode(block);
        track->AppendBlock(block);

      }
    }

    return sequence;
  }

  return nullptr;
}

OLIVE_NAMESPACE_EXIT
