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

#include <QDebug>

#include "node/block/gap/gap.h"

TimelineView::PointerTool::PointerTool(TimelineView *parent) :
  Tool(parent)
{
}

void FlipControlAndShiftModifiers(QMouseEvent* e) {
  if (e->modifiers() & Qt::ControlModifier & Qt::ShiftModifier) {
    return;
  }

  if (e->modifiers() & Qt::ShiftModifier) {
    e->setModifiers((e->modifiers() | Qt::ControlModifier) & ~Qt::ShiftModifier);
  } else if (e->modifiers() & Qt::ControlModifier) {
    e->setModifiers((e->modifiers() | Qt::ShiftModifier) & ~Qt::ControlModifier);
  }
}

void TimelineView::PointerTool::MousePress(QMouseEvent *event)
{
  FlipControlAndShiftModifiers(event);

  parent()->QGraphicsView::mousePressEvent(event);
}

void TimelineView::PointerTool::MouseMove(QMouseEvent *event)
{
  FlipControlAndShiftModifiers(event);

  parent()->QGraphicsView::mouseMoveEvent(event);

  if (!dragging_) {

    drag_start_ = GetScenePos(event->pos());
    track_start_ = parent()->SceneToTrack(drag_start_.y());

    TimelineViewRect* clicked_item = static_cast<TimelineViewRect*>(GetItemAtScenePos(drag_start_));

    // Let's see if there's anything selected to drag
    if (clicked_item != nullptr) {

      TimelineViewGhostItem::Mode trim_mode;
      if (drag_start_.x() < clicked_item->x() + clicked_item->rect().left() + 20) {
        trim_mode = TimelineViewGhostItem::kTrimIn;
      } else if (drag_start_.x() > clicked_item->x() + clicked_item->rect().right() - 20) {
        trim_mode = TimelineViewGhostItem::kTrimOut;
      } else {
        trim_mode = TimelineViewGhostItem::kMove;
      }

      QList<QGraphicsItem*> selected_items = parent()->scene_.selectedItems();

      foreach (QGraphicsItem* item, selected_items) {
        TimelineViewClipItem* clip_item = static_cast<TimelineViewClipItem*>(item);
        TimelineViewGhostItem* ghost = TimelineViewGhostItem::FromClip(clip_item);

        ghost->SetScale(parent()->scale_);

        // Determine correct mode for ghost
        if (trim_mode == TimelineViewGhostItem::kMove // Movement is indiscriminate, all the ghosts can be set to this
            || clip_item == clicked_item) { // Trimming should only be the currently clicked Block
          ghost->SetMode(trim_mode);
        } else {
          ghost->SetMode(TimelineViewGhostItem::kNone);
        }

        parent()->ghost_items_.append(ghost);
        parent()->scene_.addItem(ghost);
      }
    }

    dragging_ = true;

  } else if (!parent()->ghost_items_.isEmpty()) {
    QPointF scene_pos = GetScenePos(event->pos());

    int cursor_track = parent()->SceneToTrack(scene_pos.y());
    int track_movement = cursor_track - track_start_;

    QPointF movement = scene_pos - drag_start_;

    rational time_movement = parent()->SceneToTime(movement.x());

    // Validate movement
    time_movement = ValidateFrameMovement(time_movement, parent()->ghost_items_);
    time_movement = ValidateInTrimming(time_movement, parent()->ghost_items_);
    time_movement = ValidateOutTrimming(time_movement, parent()->ghost_items_);
    track_movement = ValidateTrackMovement(track_movement, parent()->ghost_items_);

    // Perform movement
    foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
      switch (ghost->mode()) {
      case TimelineViewGhostItem::kNone:
        break;
      case TimelineViewGhostItem::kTrimIn:
        ghost->SetInAdjustment(time_movement);
        break;
      case TimelineViewGhostItem::kTrimOut:
        ghost->SetOutAdjustment(time_movement);
        break;
      case TimelineViewGhostItem::kMove:
      {
        ghost->SetInAdjustment(time_movement);
        ghost->SetOutAdjustment(time_movement);

        ghost->SetTrackAdjustment(track_movement);
        int track = ghost->GetAdjustedTrack();
        ghost->SetY(parent()->GetTrackY(track));
        ghost->SetHeight(parent()->GetTrackHeight(track));
        break;
      }
      }
    }
  }
}

void TimelineView::PointerTool::MouseRelease(QMouseEvent *event)
{
  FlipControlAndShiftModifiers(event);

  parent()->QGraphicsView::mouseReleaseEvent(event);

  QObject block_memory_manager;

  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* b = Node::ValueToPtr<Block>(ghost->data(0));

    // Replace old Block with a new Gap
    GapBlock* gap = new GapBlock();
    gap->setParent(&block_memory_manager);
    gap->set_length(b->length());

    emit parent()->RequestReplaceBlock(b, gap, ghost->Track());
  }

  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* b = Node::ValueToPtr<Block>(ghost->data(0));

    if (ghost->mode() == TimelineViewGhostItem::kTrimIn || ghost->mode() == TimelineViewGhostItem::kTrimOut) {
      // If we were trimming, we'll need to change the length

      // If we were trimming the in point, we'll need to adjust the media in too
      if (ghost->mode() == TimelineViewGhostItem::kTrimIn) {
        b->set_media_in(b->media_in() + ghost->InAdjustment());
      }

      b->set_length(ghost->AdjustedLength());
    }

    emit parent()->RequestPlaceBlock(b, ghost->GetAdjustedIn(), ghost->GetAdjustedTrack());
  }

  parent()->ClearGhosts();

  dragging_ = false;
}
