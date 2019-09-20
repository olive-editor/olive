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

void TimelineView::ImportTool::DragEnter(QDragEnterEvent *event)
{
  QStringList mime_formats = event->mimeData()->formats();

  // Listen for MIME data from a ProjectViewModel
  if (mime_formats.contains("application/x-oliveprojectitemdata")) {
    // FIXME: Implement audio insertion

    // Data is drag/drop data from a ProjectViewModel
    QByteArray model_data = event->mimeData()->data("application/x-oliveprojectitemdata");

    // Use QDataStream to deserialize the data
    QDataStream stream(&model_data, QIODevice::ReadOnly);

    // Variables to deserialize into
    quintptr item_ptr;
    int r;

    // Set drag start position
    drag_start_ = GetScenePos(event->pos());

    // Set ghosts to start where the cursor entered
    rational ghost_start = parent()->SceneToTime(drag_start_.x() - import_pre_buffer_);

    snap_points_.clear();

    while (!stream.atEnd()) {
      stream >> r >> item_ptr;

      // Get Item object
      Item* item = reinterpret_cast<Item*>(item_ptr);

      // Check if Item is Footage
      if (item->type() == Item::kFootage) {
        // If the Item is Footage, we can create a Ghost from it
        Footage* footage = static_cast<Footage*>(item);

        StreamPtr stream = footage->stream(0);

        TimelineViewGhostItem* ghost = new TimelineViewGhostItem();

        rational footage_duration;

        if (stream->type() == Stream::kImage) {
          // Stream is essentially length-less - use config's default image length
          footage_duration = kDefaultImageLength;
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

void TimelineView::ImportTool::DragMove(QDragMoveEvent *event)
{
  if (parent()->HasGhosts()) {
    QPointF pos = GetScenePos(event->pos());
    QPointF movement = pos - drag_start_;

    rational time_movement = parent()->SceneToTime(movement.x());

    int ghost_track = parent()->SceneToTrack(pos.y());
    int ghost_y = parent()->GetTrackY(ghost_track);
    int ghost_height = parent()->GetTrackHeight(ghost_track);

    time_movement = ValidateFrameMovement(time_movement, parent()->ghost_items_);

    // If snapping is enabled, check for snap points
    if (olive::core.snapping()) {
      SnapPoint(snap_points_, &time_movement);
    }

    // Move ghosts to the mouse cursor
    foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
      ghost->SetInAdjustment(time_movement);
      ghost->SetOutAdjustment(time_movement);

      ghost->SetTrack(ghost_track);

      ghost->SetY(ghost_y);
      ghost->SetHeight(ghost_height);
    }

    event->accept();
  } else {
    event->ignore();
  }
}

void TimelineView::ImportTool::DragLeave(QDragLeaveEvent *event)
{
  if (parent()->HasGhosts()) {
    parent()->ClearGhosts();

    event->accept();
  } else {
    event->ignore();
  }
}

void TimelineView::ImportTool::DragDrop(QDropEvent *event)
{
  if (parent()->HasGhosts()) {
    // We use QObject as the parent for the nodes we create. If there is no TimelineOutput node, this object going out
    // of scope will delete the nodes. If there is, they'll become parents of the NodeGraph instead
    QObject node_memory_manager;

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
      media->SetFootage(ghost->data(TimelineViewGhostItem::kAttachedFootage).value<StreamPtr>());

      NodeParam::ConnectEdge(opacity->texture_output(), clip->texture_input());
      NodeParam::ConnectEdge(media->texture_output(), opacity->texture_input());
      NodeParam::ConnectEdge(transform->matrix_output(), media->matrix_input());

      if (event->keyboardModifiers() & Qt::ControlModifier) {
        //emit parent()->RequestInsertBlockAtTime(clip, ghost->GetAdjustedIn());
      } else {
        parent()->timeline_node_->PlaceBlock(clip, ghost->GetAdjustedIn(), ghost->Track());
      }
    }

    parent()->ClearGhosts();

    event->accept();
  } else {
    event->ignore();
  }
}
