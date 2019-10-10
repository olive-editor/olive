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

#include "widget/timelineview/timelineview.h"

#include <QMimeData>
#include <QToolTip>

#include "config/config.h"
#include "common/qtversionabstraction.h"
#include "core.h"
#include "node/distort/transform/transform.h"
#include "node/color/opacity/opacity.h"
#include "node/input/media/media.h"

TimelineView::ImportTool::ImportTool(TimelineView *parent) :
  Tool(parent)
{
  // Calculate width used for importing to give ghosts a slight lead-in so the ghosts aren't right on the cursor
  QFontMetrics fm = parent->fontMetrics();
  import_pre_buffer_ = QFontMetricsWidth(&fm, "HHHHHHHH");
}

void TimelineView::ImportTool::DragEnter(TimelineViewMouseEvent *event)
{
  QStringList mime_formats = event->GetMimeData()->formats();

  // Listen for MIME data from a ProjectViewModel
  if (mime_formats.contains("application/x-oliveprojectitemdata")) {
    // FIXME: Implement audio insertion

    // Data is drag/drop data from a ProjectViewModel
    QByteArray model_data = event->GetMimeData()->data("application/x-oliveprojectitemdata");

    // Use QDataStream to deserialize the data
    QDataStream stream(&model_data, QIODevice::ReadOnly);

    // Variables to deserialize into
    quintptr item_ptr;
    int r;

    // Set drag start position
    drag_start_ = event->GetCoordinates();

    // Set ghosts to start where the cursor entered

    rational ghost_start = drag_start_.GetFrame() - parent()->SceneToTime(import_pre_buffer_);

    snap_points_.clear();

    while (!stream.atEnd()) {
      stream >> r >> item_ptr;

      // Get Item object
      Item* item = reinterpret_cast<Item*>(item_ptr);

      // Check if Item is Footage
      if (item->type() == Item::kFootage) {
        // If the Item is Footage, we can create a Ghost from it
        Footage* footage = static_cast<Footage*>(item);

        rational footage_duration;

        // Loop through all streams in footage
        foreach (StreamPtr stream, footage->streams()) {
          // Check if this stream is compatible with this TrackList
          if (stream->type() == parent()->TrackTypeToStreamType(parent()->ConnectedTrackType())) {
            TimelineViewGhostItem* ghost = new TimelineViewGhostItem();

            if (stream->type() == Stream::kImage) {
              // Stream is essentially length-less - use config's default image length
              footage_duration = Config::Current()["DefaultStillLength"].value<rational>();
            } else {
              // Use duration from file
              footage_duration = rational(stream->timebase().numerator() * stream->duration(),
                                          stream->timebase().denominator());
            }

            ghost->SetIn(ghost_start);
            ghost->SetOut(ghost_start + footage_duration);

            snap_points_.append(ghost->In());
            snap_points_.append(ghost->Out());

            ghost->setData(TimelineViewGhostItem::kAttachedFootage, QVariant::fromValue(stream));
            ghost->SetMode(olive::timeline::kMove);

            parent()->AddGhost(ghost);
          }
        }

        // Stack each ghost one after the other
        ghost_start += footage_duration;
      }
    }

    event->accept();
  } else {
    // FIXME: Implement dropping from file
    event->ignore();
  }
}

void TimelineView::ImportTool::DragMove(TimelineViewMouseEvent *event)
{
  if (parent()->HasGhosts()) {
    rational time_movement = event->GetCoordinates().GetFrame() - drag_start_.GetFrame();

    int ghost_track = event->GetCoordinates().GetTrack();
    int ghost_y = parent()->GetTrackY(ghost_track);
    int ghost_height = parent()->GetTrackHeight(ghost_track);

    // If snapping is enabled, check for snap points
    if (olive::core.snapping()) {
      SnapPoint(snap_points_, &time_movement);
    }

    time_movement = ValidateFrameMovement(time_movement, parent()->ghost_items_);

    rational earliest_ghost = RATIONAL_MAX;

    // Move ghosts to the mouse cursor
    foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
      ghost->SetInAdjustment(time_movement);
      ghost->SetOutAdjustment(time_movement);

      ghost->SetTrack(ghost_track);

      ghost->SetY(ghost_y);
      ghost->SetHeight(ghost_height);

      earliest_ghost = qMin(earliest_ghost, ghost->GetAdjustedIn());
    }

    // Generate tooltip (showing earliest in point of imported clip)
    int64_t earliest_timestamp = olive::time_to_timestamp(earliest_ghost, parent()->timebase_);
    QString tooltip_text = olive::timestamp_to_timecode(earliest_timestamp,
                                                        parent()->timebase_,
                                                        olive::CurrentTimecodeDisplay());
    QToolTip::showText(QCursor::pos(),
                       tooltip_text,
                       parent());

    event->accept();
  } else {
    event->ignore();
  }
}

void TimelineView::ImportTool::DragLeave(QDragLeaveEvent* event)
{
  if (parent()->HasGhosts()) {
    parent()->ClearGhosts();

    event->accept();
  } else {
    event->ignore();
  }
}

void TimelineView::ImportTool::DragDrop(TimelineViewMouseEvent *event)
{
  if (parent()->HasGhosts()) {
    // We use QObject as the parent for the nodes we create. If there is no TimelineOutput node, this object going out
    // of scope will delete the nodes. If there is, they'll become parents of the NodeGraph instead
    QObject node_memory_manager;

    QUndoCommand* command = new QUndoCommand();

    foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
      ClipBlock* clip = new ClipBlock();
      MediaInput* media = new MediaInput();
      TransformDistort* transform = new TransformDistort();
      OpacityNode* opacity = new OpacityNode();

      // Set parents to node_memory_manager in case no TimelineOutput receives this signal
      clip->setParent(&node_memory_manager);
      media->setParent(&node_memory_manager);
      transform->setParent(&node_memory_manager);
      opacity->setParent(&node_memory_manager);

      clip->set_length(ghost->Length());
      StreamPtr footage_stream = ghost->data(TimelineViewGhostItem::kAttachedFootage).value<StreamPtr>();
      media->SetFootage(footage_stream);
      clip->set_block_name(footage_stream->footage()->name());

      NodeParam::ConnectEdge(opacity->texture_output(), clip->texture_input());
      NodeParam::ConnectEdge(media->texture_output(), opacity->texture_input());
      NodeParam::ConnectEdge(transform->matrix_output(), media->matrix_input());


      if (event->GetModifiers() & Qt::ControlModifier) {
        //emit parent()->RequestInsertBlockAtTime(clip, ghost->GetAdjustedIn());
      } else {
        new TrackPlaceBlockCommand(parent()->timeline_node_, ghost->Track(), clip, ghost->GetAdjustedIn(), command);
      }
    }

    olive::undo_stack.pushIfHasChildren(command);

    parent()->ClearGhosts();

    event->accept();
  } else {
    event->ignore();
  }
}
