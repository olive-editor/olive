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

#include "loadotio.h"

#ifdef USE_OTIO

#include <opentimelineio/clip.h>
#include <opentimelineio/externalReference.h>
#include <opentimelineio/gap.h>
#include <opentimelineio/serializableCollection.h>
#include <opentimelineio/timeline.h>
#include <opentimelineio/transition.h>
#include <QFileInfo>

#include "node/block/clip/clip.h"
#include "node/block/gap/gap.h"
#include "node/block/transition/crossdissolve/crossdissolvetransition.h"
#include "node/project/folder/folder.h"
#include "node/project/footage/footage.h"
#include "node/project/sequence/sequence.h"
#include "widget/timelinewidget/timelineundo.h"

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
    // Create sequence
    Sequence* sequence = new Sequence();
    sequence->SetLabel(QString::fromStdString(timeline->name()));
    sequence->setParent(project_);
    FolderAddChild(project_->root(), sequence).redo();

    // FIXME: As far as I know, OTIO doesn't store video/audio parameters?
    sequence->set_default_parameters();

    // Iterate through tracks
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

        // Create track
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

      Block* previous_block = nullptr;
      bool prev_block_transition = false;

      for (auto otio_block_retainer : clip_map) {

        auto otio_block = otio_block_retainer.value;

        Block* block = nullptr;

        if (otio_block->schema_name() == "Clip") {

          block = new ClipBlock();

        } else if (otio_block->schema_name() == "Gap") {

          block = new GapBlock();

        } else if (otio_block->schema_name() == "Transition") {

          // Todo: Look into OTIO supported transitions and add them to Olive
          block = new CrossDissolveTransition();

        } else {

          // We don't know what this is yet, just create a gap for now so that *something* is there
          qWarning() << "Found unknown block type:" << otio_block->schema_name().c_str();
          block = new GapBlock();
        }

        block->setParent(project_);
        block->SetLabel(QString::fromStdString(otio_block->name()));

        track->AppendBlock(block);

        rational start_time;
        rational duration;

        if (otio_block->schema_name() == "Clip" || otio_block->schema_name() == "Gap") {
          start_time =
              rational::fromDouble(static_cast<OTIO::Item*>(otio_block)->source_range()->start_time().to_seconds());
          duration =
              rational::fromDouble(static_cast<OTIO::Item*>(otio_block)->source_range()->duration().to_seconds());

          block->set_media_in(start_time);
          block->set_length_and_media_out(duration);
        }

        // If the previous block was a transition, connect the current block to it
        if (prev_block_transition) {
          TransitionBlock* previous_transition_block = static_cast<TransitionBlock*>(previous_block);
          Node::ConnectEdge(block, NodeInput(previous_transition_block, TransitionBlock::kInBlockInput));
          prev_block_transition = false;
        }

        if (otio_block->schema_name() == "Transition") {
          TransitionBlock* transition_block = static_cast<TransitionBlock*>(block);
          OTIO::Transition* otio_block_transition = static_cast<OTIO::Transition*>(otio_block);

          duration = rational::fromDouble((otio_block_transition->in_offset() + otio_block_transition->out_offset()).to_seconds());
          transition_block->set_length_and_media_out(duration);

          if (previous_block) {
            Node::ConnectEdge(previous_block, NodeInput(transition_block, TransitionBlock::kOutBlockInput));

            // Set how far the transition eats into the previous clip
            transition_block->set_media_in(rational::fromDouble(-otio_block_transition->out_offset().to_seconds()));
          }
          prev_block_transition = true;
        }

        // Update this after it's used but before any continue statements
        previous_block = block;

        if (otio_block->schema_name() == "Clip") {
          auto otio_clip = static_cast<OTIO::Clip*>(otio_block);
          if (!otio_clip->media_reference()) {
            continue;
          }
          if (otio_clip->media_reference()->schema_name() == "ExternalReference") {
            // Link footage
            QString footage_url = QString::fromStdString(static_cast<OTIO::ExternalReference*>(otio_clip->media_reference())->target_url());

            Footage* probed_item;

            if (imported_footage.contains(footage_url)) {
              probed_item = imported_footage.value(footage_url);
            } else {
              probed_item = new Footage(footage_url);
              imported_footage.insert(footage_url, probed_item);
              probed_item->setParent(project_);
            }

            Track::Reference reference;

            if (track->type() == Track::kVideo) {
              reference = Track::Reference(Track::kVideo, 0);
            } else {
              reference = Track::Reference(Track::kAudio, 0);
            }

            QString output_id = reference.ToString();

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

#endif // USE_OTIO
