/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "loadotio.h"

#include <opentimelineio/clip.h>
#include <opentimelineio/externalReference.h>
#include <opentimelineio/gap.h>
#include <opentimelineio/serializableCollection.h>
#include <opentimelineio/timeline.h>
#include <QFileInfo>

#include "node/block/clip/clip.h"
#include "node/block/gap/gap.h"
#include "project/item/folder/folder.h"
#include "project/item/footage/footage.h"
#include "project/item/sequence/sequence.h"
#include "widget/timelinewidget/timelineundo.h"

#define OTIO opentimelineio::v1_0

namespace olive {

LoadOTIOTask::LoadOTIOTask(const QString& s) :
  ProjectLoadBaseTask(s)
{
}

bool LoadOTIOTask::Run()
{
  OTIO::ErrorStatus es;

  auto root = OTIO::SerializableObjectWithMetadata::from_json_file(GetFilename().toStdString(), &es);

  if (es != OTIO::ErrorStatus::OK) {
    SetError(tr("Failed to load OpenTimelineIO from file \"%1\"").arg(GetFilename()));
    return false;
  }

  project_ = new Project();
  project_->set_filename(GetFilename());

  std::vector<OTIO::Timeline*> timelines;

  if (root->schema_name() == "SerializableCollection") {
    // This is a number of timelines
    std::vector<OTIO::SerializableObject::Retainer<OTIO::SerializableObject>>& root_children = static_cast<OTIO::SerializableCollection*>(root)->children();

    timelines.resize(root_children.size());
    for (size_t j=0; j<root_children.size(); j++) {
      timelines[j] = static_cast<OTIO::Timeline*>(root_children[j].value);
    }
  } else if (root->schema_name() == "Timeline") {
    // This is a single timeline
    timelines.push_back(static_cast<OTIO::Timeline*>(root));
  } else {
    // Unknown root, we don't know what to do with this
    SetError(tr("Unknown OpenTimelineIO root element"));
    return false;
  }

  // Keep track of imported footage
  QMap<QString, Footage*> imported_footage;

  foreach (auto timeline, timelines) {
    Sequence* sequence = new Sequence();
    sequence->SetLabel(QString::fromStdString(timeline->name()));
    sequence->setParent(project_->root());

    // FIXME: As far as I know, OTIO doesn't store video/audio parameters?
    sequence->set_default_parameters();

    for (auto c : timeline->tracks()->children()) {
      auto otio_track = static_cast<OTIO::Track*>(c.value);

      // Create a new track
      Track* track = nullptr;

      // Determine what kind of track it is
      if (otio_track->kind() == "Video" || otio_track->kind() == "Audio") {
        Track::Type type;

        if (otio_track->kind() == "Video") {
          type = Track::kVideo;
        } else {
          type = Track::kAudio;
        }

        TimelineAddTrackCommand t(sequence->track_list(type));
        t.redo();
        track = t.track();
      } else {
        qWarning() << "Found unknown track type:" << otio_track->kind().c_str();
        continue;
      }

      // Get clips from track
      auto clip_map = otio_track->children();
      if (es != OTIO::ErrorStatus::OK) {
        SetError(tr("Failed to load clip"));
        return false;
      }

      for (auto otio_block_retainer : clip_map) {

        auto otio_block = otio_block_retainer.value;

        Block* block = nullptr;

        if (otio_block->schema_name() == "Clip") {

          block = new ClipBlock();

        } else if (otio_block->schema_name() == "Gap") {

          block = new GapBlock();

        } else {

          // We don't know what this is yet, just create a gap for now so that *something* is there
          qWarning() << "Found unknown block type:" << otio_block->schema_name().c_str();
          block = new GapBlock();

        }

        block->SetLabel(QString::fromStdString(otio_block->name()));

        rational start_time = rational::fromDouble(static_cast<OTIO::Item*>(otio_block)->source_range()->start_time().to_seconds());
        rational duration = rational::fromDouble(static_cast<OTIO::Item*>(otio_block)->source_range()->duration().to_seconds());

        block->set_media_in(start_time);
        block->set_length_and_media_out(duration);
        block->setParent(sequence);
        track->AppendBlock(block);

        if (otio_block->schema_name() == "Clip") {
          auto otio_clip = static_cast<OTIO::Clip*>(otio_block);

          if (otio_clip->media_reference()->schema_name() == "ExternalReference") {
            // Link footage
            QString footage_url = QString::fromStdString(static_cast<OTIO::ExternalReference*>(otio_clip->media_reference())->target_url());

            Footage* probed_item;

            if (imported_footage.contains(footage_url)) {
              probed_item = imported_footage.value(footage_url);
            } else {
              probed_item = new Footage(footage_url);
              imported_footage.insert(footage_url, probed_item);
              probed_item->setParent(project_->root());
            }

            Footage::StreamReference reference;

            if (track->type() == Track::kVideo) {
              reference = Footage::StreamReference(Stream::kVideo, 0);
            } else {
              reference = Footage::StreamReference(Stream::kAudio, 0);
            }

            QString output_id = probed_item->GetStringFromReference(reference);

            Node::ConnectEdge(NodeOutput(probed_item, output_id), NodeInput(block, ClipBlock::kBufferIn));
          }
        }

      }
    }
  }

  project_->moveToThread(qApp->thread());

  return true;
}

}
