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

    // Let's see if there's anything selected to drag
    if (GetItemAtScenePos(drag_start_) != nullptr) {
      QList<QGraphicsItem*> selected_items = parent()->scene_.selectedItems();

      foreach (QGraphicsItem* item, selected_items) {
        TimelineViewClipItem* clip_item = static_cast<TimelineViewClipItem*>(item);
        TimelineViewGhostItem* ghost = TimelineViewGhostItem::FromClip(clip_item);

        ghost->SetScale(parent()->scale_);

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
    track_movement = ValidateTrackMovement(track_movement, parent()->ghost_items_);

    // Perform movement
    foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
      ghost->SetInAdjustment(time_movement);
      ghost->SetOutAdjustment(time_movement);

      ghost->SetTrackAdjustment(track_movement);
      int track = ghost->GetAdjustedTrack();
      ghost->SetY(parent()->GetTrackY(track));
      ghost->SetHeight(parent()->GetTrackHeight(track));
    }
  }
}

void TimelineView::PointerTool::MouseRelease(QMouseEvent *event)
{
  FlipControlAndShiftModifiers(event);

  parent()->QGraphicsView::mouseReleaseEvent(event);

  QObject block_memory_manager;

  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* b = Node::ValueToPtr<Block>(ghost->data());

    // Replace old Block with a new Gap
    GapBlock* gap = new GapBlock();
    gap->setParent(&block_memory_manager);
    gap->set_length(b->length());

    emit parent()->RequestReplaceBlock(b, gap, ghost->Track());
  }

  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* b = Node::ValueToPtr<Block>(ghost->data());

    emit parent()->RequestPlaceBlock(b, ghost->GetAdjustedIn(), ghost->GetAdjustedTrack());
  }

  parent()->ClearGhosts();

  dragging_ = false;
}
