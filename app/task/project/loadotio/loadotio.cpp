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

#include "loadotio.h"

#include <opentimelineio/clip.h>
#include <opentimelineio/externalReference.h>
#include <opentimelineio/gap.h>
#include <opentimelineio/serializableCollection.h>
#include <opentimelineio/timeline.h>
#include <QFileInfo>
#include <QThread>

#include "core.h"
#include "node/block/clip/clip.h"
#include "node/block/gap/gap.h"
#include "node/input/media/audio/audio.h"
#include "node/input/media/video/video.h"
#include "project/item/folder/folder.h"
#include "project/item/sequence/sequence.h"

#define OTIO opentimelineio::v1_0

OLIVE_NAMESPACE_ENTER

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

  project_ = std::make_shared<Project>();
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

  QList<SequencePtr> sequences;

  // Generate a list of sequences with the same names as the timelines.
  // Assumes each timeline has a unique name.
  foreach (auto timeline, timelines) {
    SequencePtr sequence = std::make_shared<Sequence>();
    sequence->set_name(QString::fromStdString(timeline->name()));
    // Set default params incase they aren't edited.
    sequence->set_default_parameters();
    sequences.append(sequence);
  }

  // Dialog has to be called from the main thread so we pass the list of sequences here.
  QMetaObject::invokeMethod(Core::instance(),
                            "DialogImportOTIOShow",
                            Qt::BlockingQueuedConnection,
                            Q_ARG(QList<SequencePtr>, sequences)
                            );

  // Keep track of imported footage
  QMap<QString, FootagePtr> imported_footage;

  foreach (auto timeline, timelines) {
    SequencePtr sequence;

    // Find correct sequence based on itmeline name.
    foreach(SequencePtr seq, sequences) {
      if (seq->name() == QString::fromStdString(timeline->name())) {
        sequence = seq;
        break;
      }
    }

    project_->root()->add_child(sequence);

    ViewerOutput* seq_viewer = sequence->viewer_output();

    for (auto c : timeline->tracks()->children()) {
      auto otio_track = static_cast<OTIO::Track*>(c.value);

      // Create a new track
      TrackOutput* track = nullptr;

      // Determine what kind of track it is
      if (otio_track->kind() == "Video") {
        track = seq_viewer->track_list(Timeline::kTrackTypeVideo)->AddTrack();

        if (seq_viewer->track_list(Timeline::kTrackTypeVideo)->GetTrackCount() == 1) {
          // If this is the first track, connect it to the viewer
          NodeParam::ConnectEdge(track->output(), seq_viewer->texture_input());
        }
      } else if (otio_track->kind() == "Audio") {
        track = seq_viewer->track_list(Timeline::kTrackTypeAudio)->AddTrack();

        if (seq_viewer->track_list(Timeline::kTrackTypeAudio)->GetTrackCount() == 1) {
          // If this is the first track, connect it to the viewer
          NodeParam::ConnectEdge(track->output(), seq_viewer->samples_input());
        }
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
        sequence->AddNode(block);
        track->AppendBlock(block);

        if (otio_block->schema_name() == "Clip") {
          auto otio_clip = static_cast<OTIO::Clip*>(otio_block);

          if (otio_clip->media_reference()->schema_name() == "ExternalReference") {
            // Link footage
            QString footage_url = QString::fromStdString(static_cast<OTIO::ExternalReference*>(otio_clip->media_reference())->target_url());

            FootagePtr probed_item;

            if (imported_footage.contains(footage_url)) {
              probed_item = imported_footage.value(footage_url);
            } else {
              probed_item = Decoder::ProbeMedia(project_.get(), footage_url, &IsCancelled());
              imported_footage.insert(footage_url, probed_item);
              project_->root()->add_child(probed_item);
            }

            if (probed_item && probed_item->type() == Item::kFootage) {
              MediaInput* media;
              if (track->track_type() == Timeline::kTrackTypeVideo) {
                media = new VideoInput();
                media->SetStream(probed_item->get_first_stream_of_type(Stream::kVideo));
              } else {
                media = new AudioInput();
                media->SetStream(probed_item->get_first_stream_of_type(Stream::kAudio));
              }
              sequence->AddNode(media);

              NodeParam::ConnectEdge(media->output(), static_cast<ClipBlock*>(block)->texture_input());
            } else {
              // FIXME: Add to some kind of list that we couldn't find it
            }
          }
        }

      }
    }

    sequence->moveToThread(qApp->thread());
  }

  // Ugly hack to move footage streams to main thread
  /*foreach (ItemPtr item, imported_footage) {
    if (item && item->type() == Item::kFootage) {
      foreach (StreamPtr stream, std::static_pointer_cast<Footage>(item)->streams()) {
        stream->moveToThread(qApp->thread());
      }
    }
  }*/

  project_->moveToThread(qApp->thread());

  return true;
}

OLIVE_NAMESPACE_EXIT
