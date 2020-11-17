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

#include "widget/timelinewidget/timelinewidget.h"

#include <QCheckBox>
#include <QMessageBox>
#include <QMimeData>
#include <QToolTip>

#include "config/config.h"
#include "common/qtutils.h"
#include "core.h"
#include "dialog/sequence/sequence.h"
#include "node/audio/volume/volume.h"
#include "node/generator/matrix/matrix.h"
#include "node/input/media/audio/audio.h"
#include "node/input/media/video/video.h"
#include "node/math/math/math.h"
#include "project/item/sequence/sequence.h"
#include "widget/nodeview/nodeviewundo.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

Timeline::TrackType TrackTypeFromStreamType(Stream::Type stream_type)
{
  switch (stream_type) {
  case Stream::kVideo:
    return Timeline::kTrackTypeVideo;
  case Stream::kAudio:
    return Timeline::kTrackTypeAudio;
  case Stream::kSubtitle:
    // Temporarily disabled until we figure out a better thing to do with this
    //return Timeline::kTrackTypeSubtitle;
  case Stream::kUnknown:
  case Stream::kData:
  case Stream::kAttachment:
    break;
  }

  return Timeline::kTrackTypeNone;
}

ImportTool::ImportTool(TimelineWidget *parent) :
  TimelineTool(parent)
{
  // Calculate width used for importing to give ghosts a slight lead-in so the ghosts aren't right on the cursor
  import_pre_buffer_ = QtUtils::QFontMetricsWidth(parent->fontMetrics(), "HHHHHHHH");
}

void ImportTool::DragEnter(TimelineViewMouseEvent *event)
{
  QStringList mime_formats = event->GetMimeData()->formats();

  // Listen for MIME data from a ProjectViewModel
  if (mime_formats.contains("application/x-oliveprojectitemdata")) {

    // Data is drag/drop data from a ProjectViewModel
    QByteArray model_data = event->GetMimeData()->data("application/x-oliveprojectitemdata");

    // Use QDataStream to deserialize the data
    QDataStream stream(&model_data, QIODevice::ReadOnly);

    // Variables to deserialize into
    quintptr item_ptr;
    int r;
    quint64 enabled_streams;

    // Set drag start position
    drag_start_ = event->GetCoordinates();

    snap_points_.clear();

    while (!stream.atEnd()) {
      stream >> enabled_streams >> r >> item_ptr;

      // Get Item object
      Item* item = reinterpret_cast<Item*>(item_ptr);

      // Check if Item is Footage
      if (item->type() == Item::kFootage) {

        Footage* f = static_cast<Footage*>(item);

        if (f->IsValid()) {
          // If the Item is Footage, we can create a Ghost from it
          dragged_footage_.append(DraggedFootage(f, enabled_streams));
        }

      }
    }

    PrepGhosts(drag_start_.GetFrame() - parent()->SceneToTime(import_pre_buffer_),
               drag_start_.GetTrack().index());

    event->accept();
  } else {
    // FIXME: Implement dropping from file
    event->ignore();
  }
}

void ImportTool::DragMove(TimelineViewMouseEvent *event)
{
  if (!dragged_footage_.isEmpty()) {

    if (parent()->HasGhosts()) {
      rational time_movement = event->GetFrame() - drag_start_.GetFrame();
      int track_movement = event->GetTrack().index() - drag_start_.GetTrack().index();

      time_movement = ValidateTimeMovement(time_movement);
      track_movement = ValidateTrackMovement(track_movement, parent()->GetGhostItems());

      // If snapping is enabled, check for snap points
      if (Core::instance()->snapping()) {
        parent()->SnapPoint(snap_points_, &time_movement);

        time_movement = ValidateTimeMovement(time_movement);
        track_movement = ValidateTrackMovement(track_movement, parent()->GetGhostItems());
      }

      rational earliest_ghost = RATIONAL_MAX;

      // Move ghosts to the mouse cursor
      foreach (TimelineViewGhostItem* ghost, parent()->GetGhostItems()) {
        ghost->SetInAdjustment(time_movement);
        ghost->SetOutAdjustment(time_movement);
        ghost->SetTrackAdjustment(track_movement);

        earliest_ghost = qMin(earliest_ghost, ghost->GetAdjustedIn());
      }

      // Generate tooltip (showing earliest in point of imported clip)
      int64_t earliest_timestamp = Timecode::time_to_timestamp(earliest_ghost, parent()->GetToolTipTimebase());
      QString tooltip_text = Timecode::timestamp_to_timecode(earliest_timestamp,
                                                             parent()->GetToolTipTimebase(),
                                                             Core::instance()->GetTimecodeDisplay());

      // Force tooltip to update (otherwise the tooltip won't move as written in the documentation, and could get in the way
      // of the cursor)
      QToolTip::hideText();
      QToolTip::showText(QCursor::pos(),
                         tooltip_text,
                         parent());
    }

    event->accept();
  } else {
    event->ignore();
  }
}

void ImportTool::DragLeave(QDragLeaveEvent* event)
{
  if (!dragged_footage_.isEmpty()) {
    parent()->ClearGhosts();
    dragged_footage_.clear();

    event->accept();
  } else {
    event->ignore();
  }
}

void ImportTool::DragDrop(TimelineViewMouseEvent *event)
{
  if (!dragged_footage_.isEmpty()) {
    DropGhosts(event->GetModifiers() & Qt::ControlModifier);

    event->accept();
  } else {
    event->ignore();
  }
}

void ImportTool::PlaceAt(const QList<Footage *> &footage, const rational &start, bool insert)
{
  PlaceAt(FootageToDraggedFootage(footage), start, insert);
}

void ImportTool::PlaceAt(const QList<DraggedFootage> &footage, const rational &start, bool insert)
{
  dragged_footage_ = footage;

  if (dragged_footage_.isEmpty()) {
    return;
  }

  PrepGhosts(start, 0);
  DropGhosts(insert);
}

void ImportTool::FootageToGhosts(rational ghost_start, const QList<DraggedFootage> &footage_list, const rational& dest_tb, const int& track_start)
{
  foreach (const DraggedFootage& footage, footage_list) {

    // Each stream is offset by one track per track "type", we keep track of them in this vector
    QVector<int> track_offsets(Timeline::kTrackTypeCount);
    track_offsets.fill(track_start);

    QVector<TimelineViewGhostItem*> footage_ghosts;
    rational footage_duration;
    bool contains_image_stream = false;

    quint64 enabled_streams = footage.streams();

    // Loop through all streams in footage
    foreach (StreamPtr stream, footage.footage()->streams()) {
      Timeline::TrackType track_type = TrackTypeFromStreamType(stream->type());

      quint64 cached_enabled_streams = enabled_streams;
      enabled_streams >>= 1;

      // Check if this stream has a compatible TrackList
      if (track_type == Timeline::kTrackTypeNone
          || !(cached_enabled_streams & 0x1)) {
        continue;
      }

      TimelineViewGhostItem* ghost = new TimelineViewGhostItem();

      if (stream->type() == Stream::kVideo
          && std::static_pointer_cast<VideoStream>(stream)->video_type() == VideoStream::kVideoTypeStill) {
        // Stream is essentially length-less - we may use the default still image length in config,
        // or we may use another stream's length depending on the circumstance
        contains_image_stream = true;
      } else {
        // Rescale stream duration to timeline timebase
        // Convert to rational time
        if (footage.footage()->workarea()->enabled()) {
          footage_duration = qMax(footage_duration, footage.footage()->workarea()->range().length());
          ghost->SetMediaIn(footage.footage()->workarea()->in());
        } else {
          int64_t stream_duration = Timecode::rescale_timestamp_ceil(stream->duration(), stream->timebase(), dest_tb);
          footage_duration = qMax(footage_duration, Timecode::timestamp_to_time(stream_duration, dest_tb));
        }
      }

      ghost->SetTrack(TrackReference(track_type, track_offsets.at(track_type)));

      // Increment track count for this track type
      track_offsets[track_type]++;

      ghost->SetData(TimelineViewGhostItem::kAttachedFootage, QVariant::fromValue(stream));
      ghost->SetMode(Timeline::kMove);

      footage_ghosts.append(ghost);

    }

    if (contains_image_stream && footage_duration.isNull()) {
      // Footage must ONLY be image streams so no duration value was found, use default in config
      footage_duration = Config::Current()["DefaultStillLength"].value<rational>();
    }

    foreach (TimelineViewGhostItem* ghost, footage_ghosts) {
      ghost->SetIn(ghost_start);
      ghost->SetOut(ghost_start + footage_duration);

      snap_points_.append(ghost->GetIn());
      snap_points_.append(ghost->GetOut());

      parent()->AddGhost(ghost);
    }

    // Stack each ghost one after the other
    ghost_start += footage_duration;

  }
}

void ImportTool::PrepGhosts(const rational& frame, const int& track_index)
{
  if (parent()->GetConnectedNode()) {
    FootageToGhosts(frame,
                    dragged_footage_,
                    parent()->timebase(),
                    track_index);
  }
}

void ImportTool::DropGhosts(bool insert)
{
  QUndoCommand* command = new QUndoCommand();

  NodeGraph* dst_graph = nullptr;
  ViewerOutput* viewer_node = nullptr;
  Sequence* open_sequence = nullptr;

  if (parent()->GetConnectedNode()) {
    viewer_node = parent()->GetConnectedNode();
    dst_graph = static_cast<NodeGraph*>(parent()->GetConnectedNode()->parent());
  } else {
    // There's no active timeline here, ask the user what to do

    DropWithoutSequenceBehavior behavior = static_cast<DropWithoutSequenceBehavior>(Config::Current()["DropWithoutSequenceBehavior"].toInt());

    if (behavior == kDWSAsk) {
      QCheckBox* dont_ask_again_box = new QCheckBox(QCoreApplication::translate("ImportTool", "Don't ask me again"));

      QMessageBox mbox(parent());

      mbox.setIcon(QMessageBox::Question);
      mbox.setWindowTitle(QCoreApplication::translate("ImportTool", "No Active Sequence"));
      mbox.setText(QCoreApplication::translate("ImportTool", "No sequence is currently open. Would you like to create one?"));
      mbox.setCheckBox(dont_ask_again_box);

      QPushButton* auto_params_btn = mbox.addButton(QCoreApplication::translate("ImportTool", "Automatically Detect Parameters From Footage"), QMessageBox::YesRole);
      QPushButton* manual_params_btn = mbox.addButton(QCoreApplication::translate("ImportTool", "Set Parameters Manually"), QMessageBox::NoRole);
      mbox.addButton(QMessageBox::Cancel);

      mbox.exec();

      if (mbox.clickedButton() == auto_params_btn) {
        behavior = kDWSAuto;
      } else if (mbox.clickedButton() == manual_params_btn) {
        behavior = kDWSManual;
      } else {
        behavior = kDWSDisable;
      }

      if (behavior != kDWSDisable && dont_ask_again_box->isChecked()) {
        Config::Current()["DropWithoutSequenceBehavior"] = behavior;
      }
    }

    if (behavior != kDWSDisable) {
      ProjectPtr active_project = Core::instance()->GetActiveProject();

      if (active_project) {
        SequencePtr new_sequence = Core::instance()->CreateNewSequenceForProject(active_project.get());

        new_sequence->set_default_parameters();

        bool sequence_is_valid = true;

        if (behavior == kDWSAuto) {

          QList<Footage*> footage_only;

          foreach (const DraggedFootage& df, dragged_footage_) {
            footage_only.append(df.footage());
          }

          new_sequence->set_parameters_from_footage(footage_only);

        } else {

          SequenceDialog sd(new_sequence.get(), SequenceDialog::kNew, parent());
          sd.SetUndoable(false);

          if (sd.exec() != QDialog::Accepted) {
            sequence_is_valid = false;
          }

        }

        if (sequence_is_valid) {
          new_sequence->add_default_nodes();

          new ProjectViewModel::AddItemCommand(Core::instance()->GetActiveProjectModel(),
                                               Core::instance()->GetSelectedFolderInActiveProject(),
                                               new_sequence,
                                               command);

          FootageToGhosts(0, dragged_footage_, new_sequence->video_params().time_base(), 0);

          dst_graph = new_sequence.get();
          viewer_node = new_sequence->viewer_output();

          // Set this as the sequence to open
          open_sequence = new_sequence.get();
        }
      }
    }
  }

  if (dst_graph) {

    QVector<Block*> block_items(parent()->GetGhostItems().size());

    // Check if we're inserting (only valid if we're not creating this sequence ourselves)
    if (insert && !open_sequence) {
      InsertGapsAtGhostDestination(command);
    }

    for (int i=0;i<parent()->GetGhostItems().size();i++) {
      TimelineViewGhostItem* ghost = parent()->GetGhostItems().at(i);

      StreamPtr footage_stream = ghost->GetData(TimelineViewGhostItem::kAttachedFootage).value<StreamPtr>();

      ClipBlock* clip = new ClipBlock();
      clip->set_media_in(ghost->GetMediaIn());
      clip->set_length_and_media_out(ghost->GetLength());
      clip->SetLabel(footage_stream->footage()->name());
      new NodeAddCommand(dst_graph, clip, command);

      switch (footage_stream->type()) {
      case Stream::kVideo:
      {
        VideoInput* video_input = new VideoInput();
        video_input->SetStream(footage_stream);
        new NodeAddCommand(dst_graph, video_input, command);

        new NodeEdgeAddCommand(video_input->output(), clip->texture_input(), command);

        /*
        MatrixGenerator* matrix = new MatrixGenerator();
        new NodeAddCommand(dst_graph, matrix, command);

        MathNode* multiply = new MathNode();
        multiply->SetOperation(MathNode::kOpMultiply);
        new NodeAddCommand(dst_graph, multiply, command);

        new NodeEdgeAddCommand(video_input->output(), multiply->param_a_in(), command);
        new NodeEdgeAddCommand(matrix->output(), multiply->param_b_in(), command);
        new NodeEdgeAddCommand(multiply->output(), clip->texture_input(), command);
        */
        break;
      }
      case Stream::kAudio:
      {
        AudioInput* audio_input = new AudioInput();
        audio_input->SetStream(footage_stream);
        new NodeAddCommand(dst_graph, audio_input, command);

        VolumeNode* volume_node = new VolumeNode();
        new NodeAddCommand(dst_graph, volume_node, command);

        new NodeEdgeAddCommand(audio_input->output(), volume_node->samples_input(), command);
        new NodeEdgeAddCommand(volume_node->output(), clip->texture_input(), command);
        break;
      }
      default:
        break;
      }

      new TrackPlaceBlockCommand(viewer_node->track_list(ghost->GetAdjustedTrack().type()),
                                 ghost->GetAdjustedTrack().index(),
                                 clip,
                                 ghost->GetAdjustedIn(),
                                 command);

      block_items.replace(i, clip);

      // Link any clips so far that share the same Footage with this one
      for (int j=0;j<i;j++) {
        StreamPtr footage_compare = parent()->GetGhostItems().at(j)->GetData(TimelineViewGhostItem::kAttachedFootage).value<StreamPtr>();

        if (footage_compare->footage() == footage_stream->footage()) {
          Block::Link(block_items.at(j), clip);
        }
      }
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);

  parent()->ClearGhosts();
  dragged_footage_.clear();

  if (open_sequence) {
    Core::instance()->main_window()->OpenSequence(open_sequence);
  }
}

ImportTool::DraggedFootage ImportTool::FootageToDraggedFootage(Footage *f)
{
  return DraggedFootage(f, f->get_enabled_stream_flags());
}

QList<ImportTool::DraggedFootage> ImportTool::FootageToDraggedFootage(QList<Footage *> footage)
{
  QList<DraggedFootage> df;

  foreach (Footage* f, footage) {
    df.append(FootageToDraggedFootage(f));
  }

  return df;
}

}
