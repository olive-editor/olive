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
    InitiateDrag(event->pos());

    // Set dragging to true here so no matter what, the drag isn't re-initiated until it's completed
    dragging_ = true;

  } else if (!parent()->ghost_items_.isEmpty()) {

    // We're already dragging AND we have ghosts to work with
    ProcessDrag(event->pos());

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

    parent()->timeline_node_->ReplaceBlock(b, gap, ghost->Track());
  }

  // Now we place the clips back in the timeline where the user moved them. It's legal for them to overwrite parts or
  // all of the gaps we inserted earlier
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* b = Node::ValueToPtr<Block>(ghost->data(0));

    if (ghost->mode() == olive::timeline::kTrimIn || ghost->mode() == olive::timeline::kTrimOut) {
      // If we were trimming, we'll need to change the length

      // If we were trimming the in point, we'll need to adjust the media in too
      if (ghost->mode() == olive::timeline::kTrimIn) {
        b->set_media_in(b->media_in() + ghost->InAdjustment());
      }

      b->set_length(ghost->AdjustedLength());
    }

    parent()->timeline_node_->PlaceBlock(b, ghost->GetAdjustedIn(), ghost->GetAdjustedTrack());
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

void TimelineView::PointerTool::InitiateDrag(const QPoint& mouse_pos)
{
  // Record where the drag started in timeline coordinates
  drag_start_ = GetScenePos(mouse_pos);

  // Get the item that was clicked
  TimelineViewClipItem* clicked_item = dynamic_cast<TimelineViewClipItem*>(GetItemAtScenePos(drag_start_));

  // We only initiate a pointer drag if the user actually dragged an item, otherwise if they dragged on empty space
  // QGraphicsView default behavior would initiate a rubberband drag
  if (clicked_item != nullptr) {

    // Clear snap points
    snap_points_.clear();

    // Record where the drag started in timeline coordinates
    track_start_ = parent()->SceneToTrack(drag_start_.y());

    // Determine whether we're trimming or moving based on the position of the cursor
    olive::timeline::MovementMode trim_mode = olive::timeline::kNone;

    // FIXME: Hardcoded number
    const int kTrimHandle = 20;

    if (drag_start_.x() < clicked_item->x() + kTrimHandle) {
      trim_mode = olive::timeline::kTrimIn;
    } else if (drag_start_.x() > clicked_item->x() + clicked_item->rect().right() - kTrimHandle) {
      trim_mode = olive::timeline::kTrimOut;
    } else if (movement_allowed_) {
      // Some derived classes don't allow movement
      trim_mode = olive::timeline::kMove;
    }

    // Make sure we can actually perform an action here
    if (trim_mode != olive::timeline::kNone) {
      // Convert selected items list to clips list
      QList<TimelineViewClipItem*> clips = GetSelectedClips();

      // If trimming multiple clips, we only trim the earliest in each track (trimming in) or the latest in each track
      // (trimming out). If the current clip is NOT one of these, we only trim it.
      bool multitrim_enabled = true;

      // Determine if the clicked item is the earliest/latest in the track for in/out trimming respectively
      if (trim_mode == olive::timeline::kTrimIn
          || trim_mode == olive::timeline::kTrimOut) {
        multitrim_enabled = IsClipTrimmable(clicked_item, clips, trim_mode);
      }

      // For each selected item, create a "ghost", a visual representation of the action before it gets performed
      foreach (TimelineViewClipItem* clip_item, clips) {
        // Determine correct mode for ghost
        //
        // Movement is indiscriminate, all the ghosts can be set to do this, however trimming is limited to one block
        // PER TRACK

        bool include_this_clip = true;

        if (clip_item != clicked_item
            && (trim_mode == olive::timeline::kTrimIn || trim_mode == olive::timeline::kTrimOut)) {
          include_this_clip = multitrim_enabled ? IsClipTrimmable(clip_item, clips, trim_mode) : false;
        }

        if (include_this_clip) {
          TimelineViewGhostItem* ghost = TimelineViewGhostItem::FromClip(clip_item);

          ghost->SetScale(parent()->scale_);
          ghost->SetMode(trim_mode);

          // Prepare snap points (optimizes snapping for later)
          switch (trim_mode) {
          case olive::timeline::kMove:
            snap_points_.append(ghost->In());
            snap_points_.append(ghost->Out());
            break;
          case olive::timeline::kTrimIn:
            snap_points_.append(ghost->In());
            break;
          case olive::timeline::kTrimOut:
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
}

void TimelineView::PointerTool::ProcessDrag(const QPoint &mouse_pos)
{
  // Retrieve cursor position difference
  QPointF scene_pos = GetScenePos(mouse_pos);
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
    case olive::timeline::kNone:
      break;
    case olive::timeline::kTrimIn:
      ghost->SetInAdjustment(time_movement);
      break;
    case olive::timeline::kTrimOut:
      ghost->SetOutAdjustment(time_movement);
      break;
    case olive::timeline::kMove:
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

QList<TimelineViewClipItem *> TimelineView::PointerTool::GetSelectedClips()
{
  QList<QGraphicsItem*> selected_items = parent()->scene_.selectedItems();

  // Convert selected items list to clips list
  QList<TimelineViewClipItem*> clips;
  foreach (QGraphicsItem* item, selected_items) {
    TimelineViewClipItem* clip_cast = dynamic_cast<TimelineViewClipItem*>(item);

    if (clip_cast != nullptr) {
      clips.append(clip_cast);
    }
  }

  return clips;
}

bool TimelineView::PointerTool::IsClipTrimmable(TimelineViewClipItem* clip,
                                                const QList<TimelineViewClipItem*>& items,
                                                const olive::timeline::MovementMode& mode)
{
  foreach (TimelineViewClipItem* compare, items) {
    if (clip->Track() == compare->Track()
        && clip != compare
        && ((compare->clip()->in() < clip->clip()->in() && mode == olive::timeline::kTrimIn)
            || (compare->clip()->out() > clip->clip()->out() && mode == olive::timeline::kTrimOut))) {
      return false;
    }
  }

  return true;
}
