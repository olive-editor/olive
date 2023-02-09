/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "import.h"

#include <QCheckBox>
#include <QMessageBox>
#include <QMimeData>
#include <QToolTip>

#include "config/config.h"
#include "common/qtutils.h"
#include "core.h"
#include "dialog/sequence/sequence.h"
#include "node/audio/volume/volume.h"
#include "node/block/subtitle/subtitle.h"
#include "node/distort/transform/transformdistortnode.h"
#include "node/generator/matrix/matrix.h"
#include "node/math/math/math.h"
#include "node/project/sequence/sequence.h"
#include "widget/nodeparamview/nodeparamviewundo.h"
#include "widget/nodeview/nodeviewundo.h"
#include "widget/timelinewidget/undo/timelineundopointer.h"
#include "window/mainwindow/mainwindow.h"
#include "window/mainwindow/mainwindowundo.h"

namespace olive {

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
  if (mime_formats.contains(Project::kItemMimeType)) {

    // Data is drag/drop data from a ProjectViewModel
    QByteArray model_data = event->GetMimeData()->data(Project::kItemMimeType);

    // Use QDataStream to deserialize the data
    QDataStream stream(&model_data, QIODevice::ReadOnly);

    // Variables to deserialize into
    quintptr item_ptr;
    QVector<Track::Reference> enabled_streams;

    // Set drag start position
    drag_start_ = event->GetCoordinates();

    snap_points_.clear();

    while (!stream.atEnd()) {
      stream >> enabled_streams >> item_ptr;

      // Get Item object
      Node* item = reinterpret_cast<Node*>(item_ptr);

      // Check if Item is Footage
      ViewerOutput* f = dynamic_cast<ViewerOutput*>(item);

      if (f && f->GetTotalStreamCount()) {
        // If the Item is Footage, we can create a Ghost from it
        dragged_footage_.append({f, enabled_streams});
      }
    }

    // Create a reasonable amount of space to inset the cursor by when importing
    ghost_offset_ = drag_start_.GetFrame();

    if (!event->GetBypassImportBuffer()) {
      ghost_offset_ -= parent()->SceneToTime(import_pre_buffer_);
    }

    PrepGhosts(ghost_offset_, drag_start_.GetTrack().index());

    if (parent()->HasGhosts() || !parent()->GetConnectedNode()) {
      // We only clear the tentative track if the mimedata is about to be destroyed (i.e. the drag
      // is cancelled). If we do this in DragLeave, it leads to undesirable behavior if the cursor
      // is going between views (subtitle track rapidly appearing and disappearing)
      QObject::connect(event->GetMimeData(), &QObject::destroyed, parent(), &TimelineWidget::ClearTentativeSubtitleTrack);

      event->accept();
    } else {
      event->ignore();
    }
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

      // Keep ghost offset no lower than 0
      if (ghost_offset_ + time_movement < 0) {
        time_movement = -ghost_offset_;
      }

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
      rational tooltip_timebase = parent()->GetTimebaseForTrackType(event->GetTrack().type());
      QString tooltip_text = QString::fromStdString(Timecode::time_to_timecode(earliest_ghost,
                                                                               tooltip_timebase,
                                                                               Core::instance()->GetTimecodeDisplay()));

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
    auto command = new MultiUndoCommand();
    DropGhosts(event->GetModifiers() & Qt::ControlModifier, command);
    Core::instance()->undo_stack()->pushIfHasChildren(command);

    event->accept();
  } else {
    event->ignore();
  }
}

void ImportTool::PlaceAt(const QVector<ViewerOutput *> &footage, const rational &start, bool insert, MultiUndoCommand *command, int track_offset)
{
  DraggedFootageData refs;

  foreach (ViewerOutput* f, footage) {
    refs.append({f, f->GetEnabledStreamsAsReferences()});
  }

  PlaceAt(refs, start, insert, command, track_offset);
}

void ImportTool::PlaceAt(const DraggedFootageData &footage, const rational &start, bool insert, MultiUndoCommand *command, int track_offset)
{
  dragged_footage_ = footage;

  if (dragged_footage_.isEmpty()) {
    return;
  }

  PrepGhosts(start, track_offset);
  DropGhosts(insert, command);
}

void ImportTool::FootageToGhosts(rational ghost_start, const DraggedFootageData &sorted, const rational& dest_tb, const int& track_start)
{
  for (auto it=sorted.cbegin(); it!=sorted.cend(); it++) {
    ViewerOutput* footage = it->first;

    if (footage == sequence() || (sequence() && footage->InputsFrom(sequence(), true))) {
      // Prevent cyclical dependency
      continue;
    }

    // Each stream is offset by one track per track "type", we keep track of them in this vector
    QVector<int> track_offsets(Track::kCount);
    track_offsets.fill(track_start);

    rational footage_duration;
    rational ghost_in;

    TimelineWorkArea* wk = footage->GetWorkArea();
    if (wk->enabled()) {
      footage_duration = wk->length();
      ghost_in = wk->in();
    } else {
      footage_duration = footage->GetLength();

      if (footage_duration.isNull()) {
        // Fallback to still length if legngth was 0
        footage_duration = OLIVE_CONFIG("DefaultStillLength").value<rational>();
      }
    }

    // Snap footage duration to timebase
    rational snap_mvmt = SnapMovementToTimebase(footage_duration, 0, dest_tb);
    if (!snap_mvmt.isNull()) {
      footage_duration += snap_mvmt;
    }

    // Create ghosts
    foreach (const Track::Reference& ref, it->second) {
      Track::Type track_type = ref.type();
      Track::Reference dest_track(track_type, track_offsets.at(track_type));

      if (track_type == Track::kVideo || track_type == Track::kAudio) {
        auto ghost = CreateGhost(TimeRange(ghost_start, ghost_start + footage_duration), ghost_in, dest_track);

        // Increment track count for this track type
        track_offsets[track_type]++;

        TimelineViewGhostItem::AttachedFootage af = {it->first, ref.ToString()};
        ghost->SetData(TimelineViewGhostItem::kAttachedFootage, QVariant::fromValue(af));
      } else if (track_type == Track::kSubtitle) {
        SubtitleParams sp = footage->GetSubtitleParams(ref.index());

        for (const Subtitle &sub : sp) {
          auto ghost = CreateGhost(sub.time() + ghost_start, 0, dest_track);

          ghost->SetData(TimelineViewGhostItem::kAttachedFootage, QVariant::fromValue(sub));
        }

        parent()->AddTentativeSubtitleTrack();
      }
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
                    parent()->GetConnectedNode()->GetVideoParams().time_base(),
                    track_index);
  }
}

void ImportTool::DropGhosts(bool insert, MultiUndoCommand *parent_command)
{
  auto command = new MultiUndoCommand();

  if (MultiUndoCommand *c = parent()->TakeSubtitleSectionCommand()) {
    command->add_child(c);
  }

  NodeGraph* dst_graph = nullptr;
  Sequence* sequence = this->sequence();
  bool open_sequence = false;

  if (sequence) {
    dst_graph = sequence->parent();
  } else {
    // There's no active timeline here, ask the user what to do

    DropWithoutSequenceBehavior behavior = static_cast<DropWithoutSequenceBehavior>(OLIVE_CONFIG("DropWithoutSequenceBehavior").toInt());

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
        OLIVE_CONFIG("DropWithoutSequenceBehavior") = behavior;
      }
    }

    if (behavior != kDWSDisable) {
      Project* active_project = Core::instance()->GetActiveProject();

      if (active_project) {
        Sequence* new_sequence = Core::instance()->CreateNewSequenceForProject(active_project);

        new_sequence->set_default_parameters();

        bool sequence_is_valid = true;

        // Even if the user selected manual, set from footage anyway so the user has a useful
        // starting point
        QVector<ViewerOutput*> footage_only;

        for (auto it=dragged_footage_.cbegin(); it!=dragged_footage_.cend(); it++) {
          if (!footage_only.contains(it->first)) {
            footage_only.append(it->first);
          }
        }

        new_sequence->set_parameters_from_footage(footage_only);

        // If the user selected manual, show them a dialog with parameters
        if (behavior == kDWSManual) {

          SequenceDialog sd(new_sequence, SequenceDialog::kNew, parent());
          sd.SetUndoable(false);

          if (sd.exec() != QDialog::Accepted) {
            sequence_is_valid = false;
          }

        }

        if (sequence_is_valid) {
          dst_graph = Core::instance()->GetActiveProject();

          command->add_child(new NodeAddCommand(dst_graph, new_sequence));
          command->add_child(new FolderAddChild(Core::instance()->GetSelectedFolderInActiveProject(), new_sequence));
          command->add_child(new NodeSetPositionCommand(new_sequence, new_sequence, QPointF(0, 0)));
          new_sequence->add_default_nodes(command);

          FootageToGhosts(0, dragged_footage_, new_sequence->GetVideoParams().time_base(), 0);

          if (MultiUndoCommand *c = parent()->TakeSubtitleSectionCommand()) {
            command->add_child(c);
          }

          sequence = new_sequence;

          // Set this as the sequence to open
          open_sequence = true;
        } else {
          // If the sequence is valid, ownership is passed to AddItemCommand.
          // Otherwise, we're responsible for deleting it.
          delete new_sequence;
        }
      }
    }
  }

  std::list<ClipBlock*> imported_clips;

  if (dst_graph) {

    QVector<Block*> block_items(parent()->GetGhostItems().size());

    // Check if we're inserting (only valid if we're not creating this sequence ourselves)
    if (insert && !open_sequence) {
      InsertGapsAtGhostDestination(command);
    }

    for (int i=0;i<parent()->GetGhostItems().size();i++) {
      TimelineViewGhostItem* ghost = parent()->GetGhostItems().at(i);
      Block* block = nullptr;

      Track::Type track_type = ghost->GetAdjustedTrack().type();
      if (track_type == Track::kVideo || track_type == Track::kAudio) {
        TimelineViewGhostItem::AttachedFootage footage_stream = ghost->GetData(TimelineViewGhostItem::kAttachedFootage).value<TimelineViewGhostItem::AttachedFootage>();

        ClipBlock* clip = new ClipBlock();
        block = clip;
        clip->set_media_in(ghost->GetMediaIn());
        clip->SetLabel(footage_stream.footage->GetLabel());
        command->add_child(new NodeAddCommand(dst_graph, clip));

        // Position clip in its own context
        command->add_child(new NodeSetPositionCommand(clip, clip, QPointF(0, 0)));

        int dep_pos = kDefaultDistanceFromOutput;

        // Position footage in its context
        command->add_child(new NodeSetPositionCommand(footage_stream.footage, clip, QPointF(dep_pos, 0)));

        dep_pos++;

        switch (Track::Reference::TypeFromString(footage_stream.output)) {
        case Track::kVideo:
        {
          TransformDistortNode* transform = new TransformDistortNode();
          command->add_child(new NodeAddCommand(dst_graph, transform));

          command->add_child(new NodeSetValueHintCommand(transform, TransformDistortNode::kTextureInput, -1, Node::ValueHint({NodeValue::kTexture}, footage_stream.output)));

          command->add_child(new NodeEdgeAddCommand(footage_stream.footage, NodeInput(transform, TransformDistortNode::kTextureInput)));
          command->add_child(new NodeEdgeAddCommand(transform, NodeInput(clip, ClipBlock::kBufferIn)));
          command->add_child(new NodeSetPositionCommand(transform, clip, QPointF(dep_pos, 0)));
          break;
        }
        case Track::kAudio:
        {
          VolumeNode* volume_node = new VolumeNode();
          command->add_child(new NodeAddCommand(dst_graph, volume_node));

          command->add_child(new NodeSetValueHintCommand(volume_node, VolumeNode::kSamplesInput, -1, Node::ValueHint({NodeValue::kSamples}, footage_stream.output)));

          command->add_child(new NodeEdgeAddCommand(footage_stream.footage, NodeInput(volume_node, VolumeNode::kSamplesInput)));
          command->add_child(new NodeEdgeAddCommand(volume_node, NodeInput(clip, ClipBlock::kBufferIn)));
          command->add_child(new NodeSetPositionCommand(volume_node, clip, QPointF(dep_pos, 0)));
          break;
        }
        default:
          break;
        }

        // Link any clips so far that share the same Footage with this one
        for (int j=0;j<i;j++) {
          TimelineViewGhostItem::AttachedFootage footage_compare = parent()->GetGhostItems().at(j)->GetData(TimelineViewGhostItem::kAttachedFootage).value<TimelineViewGhostItem::AttachedFootage>();

          if (footage_compare.footage == footage_stream.footage) {
            Block::Link(block_items.at(j), clip);
          }
        }

        imported_clips.push_back(clip);
      } else if (track_type == Track::kSubtitle) {
        Subtitle src = ghost->GetData(TimelineViewGhostItem::kAttachedFootage).value<Subtitle>();
        SubtitleBlock *sub = new SubtitleBlock();
        sub->SetText(src.text());
        block = sub;

        command->add_child(new NodeAddCommand(dst_graph, sub));
        command->add_child(new NodeSetPositionCommand(sub, sub, QPointF(0, 0)));
      }

      block->set_length_and_media_out(ghost->GetLength());

      command->add_child(new TrackPlaceBlockCommand(sequence->track_list(ghost->GetAdjustedTrack().type()),
                                                    ghost->GetAdjustedTrack().index(),
                                                    block,
                                                    ghost->GetAdjustedIn()));

      block_items.replace(i, block);
    }
  }

  if (open_sequence) {
    command->add_child(new OpenSequenceCommand(sequence));
  }

  // Do command now because RequestInvalidatedFromConnected relies on track type, which will be
  // "none" before this command is done because it won't be connected to any track
  command->redo_now();
  parent_command->add_child(command);

  while (!imported_clips.empty()) {
    imported_clips.front()->RequestInvalidatedFromConnected();
    imported_clips.pop_front();
  }

  parent()->ClearGhosts();
  dragged_footage_.clear();
}

TimelineViewGhostItem* ImportTool::CreateGhost(const TimeRange &range, const rational &media_in, const Track::Reference &track)
{
  TimelineViewGhostItem* ghost = new TimelineViewGhostItem();

  ghost->SetIn(range.in());
  ghost->SetOut(range.out());
  ghost->SetMediaIn(media_in);
  ghost->SetTrack(track);

  snap_points_.push_back(ghost->GetIn());
  snap_points_.push_back(ghost->GetOut());

  ghost->SetMode(Timeline::kMove);

  parent()->AddGhost(ghost);

  return ghost;
}

}
