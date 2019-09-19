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

#include "common/range.h"
#include "core.h"
#include "node/block/gap/gap.h"

TimelineView::PointerTool::PointerTool(TimelineView *parent) :
  Tool(parent),
  movement_allowed_(true)
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
  // We use Shift for multiple selection while Qt uses Ctrl, we flip those modifiers here to compensate
  FlipControlAndShiftModifiers(event);

  // Default QGraphicsView behavior (item selection)
  parent()->QGraphicsView::mousePressEvent(event);

  // We don't initiate dragging here since clicking could easily be just for selecting
}

void TimelineView::PointerTool::MouseMove(QMouseEvent *event)
{
  // We use Shift for multiple selection while Qt uses Ctrl, we flip those modifiers here to compensate
  FlipControlAndShiftModifiers(event);

  // Default QGraphicsView behavior (item selection)
  parent()->QGraphicsView::mouseMoveEvent(event);

  // Now that the cursor has moved, we will assume the intention is to drag
  if (!dragging_) {

    // If we haven't started dragging yet, we'll initiate a drag here

    // Record where the drag started in timeline coordinates
    drag_start_ = GetScenePos(event->pos());

    // Get the item that was clicked
    TimelineViewRect* clicked_item = dynamic_cast<TimelineViewRect*>(GetItemAtScenePos(drag_start_));

    // We only initiate a pointer drag if the user actually dragged an item, otherwise if they dragged on empty space
    // QGraphicsView default behavior would initiate a rubberband drag

    if (clicked_item != nullptr) {

      // Clear snap points
      snap_points_.clear();

      // Record where the drag started in timeline coordinates
      track_start_ = parent()->SceneToTrack(drag_start_.y());

      // Determine whether we're trimming or moving based on the position of the cursor
      TimelineViewGhostItem::Mode trim_mode = TimelineViewGhostItem::kNone;

      // FIXME: Hardcoded number
      const int kTrimHandle = 20;



      if (drag_start_.x() < clicked_item->x() + kTrimHandle) {
        trim_mode = TimelineViewGhostItem::kTrimIn;
      } else if (drag_start_.x() > clicked_item->x() + clicked_item->rect().right() - kTrimHandle) {
        trim_mode = TimelineViewGhostItem::kTrimOut;
      } else if (movement_allowed_) {
        // Some derived classes don't allow movement
        trim_mode = TimelineViewGhostItem::kMove;
      }

      // Make sure we can actually perform an action here
      if (trim_mode != TimelineViewGhostItem::kNone) {
        QList<QGraphicsItem*> selected_items = parent()->scene_.selectedItems();

        // If trimming multiple clips, we only trim the earliest in each track (trimming in) or the latest in each track
        // (trimming out). If the current clip is NOT one of these, we only trim it.
        bool multitrim_enabled = true;

        // Determine if the clicked item is the earliest/latest in the track for in/out trimming respectively
        if (trim_mode == TimelineViewGhostItem::kTrimIn || trim_mode == TimelineViewGhostItem::kTrimOut) {
          TimelineViewClipItem* clicked_clip = static_cast<TimelineViewClipItem*>(clicked_item);

          foreach (QGraphicsItem* item, selected_items) {
            TimelineViewClipItem* clip_item = static_cast<TimelineViewClipItem*>(item);

            if (clip_item != clicked_item
                && clip_item->Track() == clicked_item->Track()
                && ((trim_mode == TimelineViewGhostItem::kTrimIn && clip_item->clip()->in() < clicked_clip->clip()->in())
                    || (trim_mode == TimelineViewGhostItem::kTrimOut && clip_item->clip()->out() > clicked_clip->clip()->out()))) {
              multitrim_enabled = false;
              break;
            }
          }
        }

        // For each selected item, create a "ghost", a visual representation of the action before it gets performed
        foreach (QGraphicsItem* item, selected_items) {
          TimelineViewClipItem* clip_item = dynamic_cast<TimelineViewClipItem*>(item);


          // Determine correct mode for ghost
          //
          // Movement is indiscriminate, all the ghosts can be set to do this, however trimming is limited to one block
          // PER TRACK

          bool include_this_clip = true;

          if (clip_item != clicked_item
              && (trim_mode == TimelineViewGhostItem::kTrimIn || trim_mode == TimelineViewGhostItem::kTrimOut)) {
            if (multitrim_enabled) {
              // Determine if this clip is the earliest/latest in its track
              foreach (QGraphicsItem* item, selected_items) {
                TimelineViewClipItem* test_clip = static_cast<TimelineViewClipItem*>(item);

                if (clip_item != test_clip
                    && clip_item->Track() == test_clip->Track()
                    && ((trim_mode == TimelineViewGhostItem::kTrimIn && test_clip->clip()->in() < clip_item->clip()->in())
                        || (trim_mode == TimelineViewGhostItem::kTrimOut && test_clip->clip()->out() > clip_item->clip()->out()))) {
                  include_this_clip = false;
                  break;
                }
              }
            } else {
              include_this_clip = false;
            }
          }

          if (include_this_clip) {
            TimelineViewGhostItem* ghost = TimelineViewGhostItem::FromClip(clip_item);

            ghost->SetScale(parent()->scale_);
            ghost->SetMode(trim_mode);

            // Prepare snap points (optimizes snapping for later)
            switch (trim_mode) {
            case TimelineViewGhostItem::kMove:
              snap_points_.append(ghost->In());
              snap_points_.append(ghost->Out());
              break;
            case TimelineViewGhostItem::kTrimIn:
              snap_points_.append(ghost->In());
              break;
            case TimelineViewGhostItem::kTrimOut:
              snap_points_.append(ghost->Out());
              break;
            default:
              break;
            }

            parent()->ghost_items_.append(ghost);
            parent()->scene_.addItem(ghost);
          }
        }
      }
    }

    // Set dragging to true here so no matter what, the drag isn't re-initiated until it's completed
    dragging_ = true;

  } else if (!parent()->ghost_items_.isEmpty()) {

    // We're already dragging AND we have ghosts to work with

    // Retrieve cursor position difference
    QPointF scene_pos = GetScenePos(event->pos());
    QPointF movement = scene_pos - drag_start_;

    // Determine track movement
    int cursor_track = parent()->SceneToTrack(scene_pos.y());
    int track_movement = cursor_track - track_start_;

    // Determine frame movement
    rational time_movement = parent()->SceneToTime(movement.x());

    // Validate movement (enforce all ghosts moving in legal ways)
    time_movement = FrameValidateInternal(time_movement, parent()->ghost_items_);
    track_movement = ValidateTrackMovement(track_movement, parent()->ghost_items_);

    // Perform snapping if enabled (adjusts time_movement if it's close to any potential snap points)
    if (olive::core.snapping()) {
      SnapPoint(snap_points_, &time_movement);
    }

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

        // Track movement is only legal for moving, not for trimming
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
  // We use Shift for multiple selection while Qt uses Ctrl, we flip those modifiers here to compensate
  FlipControlAndShiftModifiers(event);

  // Default QGraphicsView behavior (item selection)
  parent()->QGraphicsView::mouseReleaseEvent(event);

  MouseReleaseInternal(event);

  if (dragging_) {
    parent()->ClearGhosts();
    snap_points_.clear();
  }

  dragging_ = false;
}

void TimelineView::PointerTool::SetMovementAllowed(bool allowed)
{
  movement_allowed_ = allowed;
}

void TimelineView::PointerTool::MouseReleaseInternal(QMouseEvent *event)
{
  Q_UNUSED(event)

  // We create a QObject on the stack so that when we allocate objects on the heap, they aren't parent-less and will
  // get cleaned up if they aren't re-parented by the attached NodeGraph
  QObject block_memory_manager;

  // Since all the ghosts will be leaving their old position in some way, we replace all of them with gaps here so the
  // entire timeline isn't disrupted in the process
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* b = Node::ValueToPtr<Block>(ghost->data(0));

    // Replace old Block with a new Gap
    GapBlock* gap = new GapBlock();
    gap->setParent(&block_memory_manager);
    gap->set_length(b->length());

    emit parent()->RequestReplaceBlock(b, gap, ghost->Track());
  }

  // Now we place the clips back in the timeline where the user moved them. It's legal for them to overwrite parts or
  // all of the gaps we inserted earlier
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
}

rational TimelineView::PointerTool::FrameValidateInternal(rational time_movement, QVector<TimelineViewGhostItem *>)
{
  // Default behavior is to validate all movement and trimming
  time_movement = ValidateFrameMovement(time_movement, parent()->ghost_items_);
  time_movement = ValidateInTrimming(time_movement, parent()->ghost_items_);
  time_movement = ValidateOutTrimming(time_movement, parent()->ghost_items_);

  return time_movement;
}
