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
#include <QApplication>
#include <QFileInfo>
#include <QThread>

#include "core.h"
#include "node/audio/volume/volume.h"
#include "node/block/clip/clip.h"
#include "node/block/gap/gap.h"
#include "node/block/transition/crossdissolve/crossdissolvetransition.h"
#include "node/distort/transform/transformdistortnode.h"
#include "node/generator/matrix/matrix.h"
#include "node/math/math/math.h"
#include "node/project/folder/folder.h"
#include "node/project/footage/footage.h"
#include "node/project/sequence/sequence.h"
#include "window/mainwindow/mainwindowundo.h"
#include "widget/nodeview/nodeviewundo.h"
#include "widget/timelinewidget/undo/timelineundogeneral.h"

namespace olive {

LoadOTIOTask::LoadOTIOTask(const QString& s) :
  ProjectLoadBaseTask(s)
{
}

bool LoadOTIOTask::Run()
{
  OTIO::ErrorStatus es;

  auto root = OTIO::SerializableObjectWithMetadata::from_json_file(GetFilename().toStdString(), &es);

  if (es.outcome != OTIO::ErrorStatus::Outcome::OK) {
    SetError(tr("Failed to load OpenTimelineIO from file \"%1\"").arg(GetFilename()));
    return false;
  }

  project_ = new Project();
  project_->set_modified(true);

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
  QMap<OTIO::Timeline*, Sequence*> timeline_sequnce_map;

  // Variables used for loading bar
  float number_of_clips = 0;
  float clips_done = 0;

  // Generate a list of sequences with the same names as the timelines.
  // Assumes each timeline has a unique name.
  int unnamed_sequence_count = 0;
  foreach (auto timeline, timelines) {
    Sequence* sequence = new Sequence();
    if (!timeline->name().empty()) {
      sequence->SetLabel(QString::fromStdString(timeline->name()));
    } else {
      // If the otio timeline does not provide a name, create a default one here
      unnamed_sequence_count++;
      QString label = tr("Sequence %1").arg(unnamed_sequence_count);
      sequence->SetLabel(QString::fromStdString(label.toStdString()));
    }
    // Set default params incase they aren't edited.
    sequence->set_default_parameters();
    timeline_sequnce_map.insert(timeline, sequence);

    // Get number of clips for loading bar
    foreach (auto track, timeline->tracks()->children()) {
      auto otio_track = static_cast<OTIO::Track*>(track.value);
      number_of_clips += otio_track->children().size();
    }
  }

  // Dialog has to be called from the main thread so we pass the list of sequences here.
  bool accepted = false;
  QMetaObject::invokeMethod(Core::instance(),
                            "DialogImportOTIOShow",
                            Qt::BlockingQueuedConnection,
                            Q_RETURN_ARG(bool, accepted),
                            Q_ARG(QList<Sequence*>,timeline_sequnce_map.values()));

  if (!accepted) {
    // Cancel to indicate to caller that this task did not complete and to simply dispose of it
    Cancel();
    qDeleteAll(timeline_sequnce_map); // Clear sequences
    return true;
  }

  foreach (auto timeline, timeline_sequnce_map.keys()) {
    Sequence* sequence = timeline_sequnce_map.value(timeline);
    sequence->setParent(project_);
    FolderAddChild(project_->root(), sequence).redo_now();

    // Create a folder for this sequence's footage
    Folder* sequence_footage = new Folder();
    sequence_footage->SetLabel(QString::fromStdString(timeline->name()));
    sequence_footage->setParent(project_);
    FolderAddChild(project_->root(), sequence_footage).redo_now();

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
        t.redo_now();
        track = t.track();
      } else {
        qWarning() << "Found unknown track type:" << otio_track->kind().c_str();
        continue;
      }

      // Get clips from track
      auto clip_map = otio_track->children();
      if (es.outcome != OTIO::ErrorStatus::Outcome::OK) {
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

          if (otio_block->schema_name() == "Clip") {
            static_cast<ClipBlock*>(block)->set_media_in(start_time);
          }
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

          // Set how far the transition eats into the previous clip
          transition_block->set_offsets_and_length(rational::fromRationalTime(otio_block_transition->in_offset()), rational::fromRationalTime(otio_block_transition->out_offset()));

          if (previous_block) {
            Node::ConnectEdge(previous_block, NodeInput(transition_block, TransitionBlock::kOutBlockInput));
          }
          prev_block_transition = true;

          // Add nodes to the graph and set up contexts
          block->setParent(sequence->parent());

          // Position transition in its own context
          block->SetNodePositionInContext(block, QPointF(0, 0));
        }

        if (otio_block->schema_name() == "Gap") {
          // Add nodes to the graph and set up contexts
          block->setParent(sequence->parent());

          // Position transition in its own context
          block->SetNodePositionInContext(block, QPointF(0, 0));
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


              QFileInfo info(probed_item->filename());
              probed_item->SetLabel(info.fileName());

              FolderAddChild add(sequence_footage, probed_item);
              add.redo_now();
            }

            // Add nodes to the graph and set up contexts
            block->setParent(sequence->parent());

            // Position clip in its own context
            block->SetNodePositionInContext(block, QPointF(0, 0));

            // Position footage in its context
            block->SetNodePositionInContext(probed_item, QPointF(-2, 0));


            if (track->type() == Track::kVideo) {
              TransformDistortNode* transform = new TransformDistortNode();
              transform->setParent(sequence->parent());

              Node::ConnectEdge(probed_item, NodeInput(transform, TransformDistortNode::kTextureInput));
              Node::ConnectEdge(transform, NodeInput(block, ClipBlock::kBufferIn));
              block->SetNodePositionInContext(transform, QPointF(-1, 0));
            } else {
              VolumeNode* volume_node = new VolumeNode();
              volume_node->setParent(sequence->parent());

              Node::ConnectEdge(probed_item, NodeInput(volume_node, VolumeNode::kSamplesInput));
              Node::ConnectEdge(volume_node, NodeInput(block, ClipBlock::kBufferIn));
              block->SetNodePositionInContext(volume_node, QPointF(-1, 0));
            }
          }
        }
        clips_done++;
        emit ProgressChanged(clips_done / number_of_clips);
      }
    }
  }

  project_->moveToThread(qApp->thread());

  return true;
}

}

#endif // USE_OTIO
