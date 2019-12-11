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

#include "widget/timelinewidget/timelinewidget.h"

#include <QDebug>
#include <QToolTip>

#include "common/clamp.h"
#include "common/flipmodifiers.h"
#include "common/range.h"
#include "common/timecodefunctions.h"
#include "config/config.h"
#include "core.h"
#include "node/block/gap/gap.h"

TimelineWidget::PointerTool::PointerTool(TimelineWidget *parent) :
  Tool(parent),
  movement_allowed_(true),
  trimming_allowed_(true),
  track_movement_allowed_(true),
  rubberband_selecting_(false)
{
}

void TimelineWidget::PointerTool::MousePress(TimelineViewMouseEvent *event)
{
  // Main selection code

  TimelineViewBlockItem* item = GetItemAtScenePos(event->GetCoordinates());

  bool selectable_item = (item != nullptr
      && item->flags() & QGraphicsItem::ItemIsSelectable
      && !parent()->GetTrackFromReference(item->Track())->IsLocked());

  if (selectable_item) {
    // Cache the clip's type for use later
    drag_track_type_ = item->Track().type();
  }

  // If this item is already selected
  if (selectable_item
      && item->isSelected()) {

    // If shift is held, deselect it
    if (event->GetModifiers() & Qt::ShiftModifier) {
      item->setSelected(false);

      // If not holding alt, deselect all links as well
      if (!(event->GetModifiers() & Qt::AltModifier)) {
        parent()->SetBlockLinksSelected(item->block(), false);
      }
    }

    // Otherwise do nothing
    return;
  }

  // If not holding shift, deselect all clips
  if (!(event->GetModifiers() & Qt::ShiftModifier)) {
    parent()->DeselectAll();
  }

  if (selectable_item) {
    // Select this item
    item->setSelected(true);

    // If not holding alt, select all links as well
    if (!(event->GetModifiers() & Qt::AltModifier)) {
      parent()->SetBlockLinksSelected(item->block(), true);
    }
  } else {
    // Start rubberband drag
    parent()->StartRubberBandSelect(!(event->GetModifiers() & Qt::AltModifier));

    rubberband_selecting_ = true;
  }
}

void TimelineWidget::PointerTool::MouseMove(TimelineViewMouseEvent *event)
{
  if (rubberband_selecting_) {

    // Process rubberband select
    parent()->MoveRubberBandSelect(!(event->GetModifiers() & Qt::AltModifier));

  } else if (!dragging_) {
    // Now that the cursor has moved, we will assume the intention is to drag

    // If we haven't started dragging yet, we'll initiate a drag here
    InitiateDrag(event->GetCoordinates());

    // Set dragging to true here so no matter what, the drag isn't re-initiated until it's completed
    dragging_ = true;

  } else if (!parent()->ghost_items_.isEmpty()) {

    // We're already dragging AND we have ghosts to work with
    ProcessDrag(event->GetCoordinates());

  }
}

void TimelineWidget::PointerTool::MouseRelease(TimelineViewMouseEvent *event)
{
  if (rubberband_selecting_) {
    parent()->EndRubberBandSelect(!(event->GetModifiers() & Qt::AltModifier));

    rubberband_selecting_ = false;
    return;
  }

  if (!parent()->ghost_items_.isEmpty()) {
    MouseReleaseInternal(event);
  }

  if (dragging_) {
    parent()->ClearGhosts();
    snap_points_.clear();
  }

  dragging_ = false;
}

void TimelineWidget::PointerTool::SetMovementAllowed(bool allowed)
{
  movement_allowed_ = allowed;
}

void TimelineWidget::PointerTool::SetTrackMovementAllowed(bool allowed)
{
  track_movement_allowed_ = allowed;
}

void TimelineWidget::PointerTool::SetTrimmingAllowed(bool allowed)
{
  trimming_allowed_ = allowed;
}

void TimelineWidget::PointerTool::MouseReleaseInternal(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)

  // We create a QObject on the stack so that when we allocate objects on the heap, they aren't parent-less and will
  // get cleaned up if they aren't re-parented by the attached NodeGraph
  QObject block_memory_manager;

  QUndoCommand* command = new QUndoCommand();

  // Since all the ghosts will be leaving their old position in some way, we replace all of them with gaps here so the
  // entire timeline isn't disrupted in the process
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* b = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    // Replace old Block with a new Gap
    GapBlock* gap = new GapBlock();
    gap->setParent(&block_memory_manager);
    gap->set_length(b->length());

    new TrackReplaceBlockCommand(parent()->GetTrackFromReference(ghost->Track()),
                                 b,
                                 gap,
                                 command);
  }

  // Now we place the clips back in the timeline where the user moved them. It's legal for them to overwrite parts or
  // all of the gaps we inserted earlier
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* b = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    // Normal blocks work in conjunction with the gap made above
    if (ghost->mode() == olive::timeline::kTrimIn || ghost->mode() == olive::timeline::kTrimOut) {
      // If we were trimming, we'll need to change the length

      // If we were trimming the in point, we'll need to adjust the media in too
      if (ghost->mode() == olive::timeline::kTrimIn) {
        new BlockResizeWithMediaInCommand(b, ghost->AdjustedLength(), command);
      } else {
        new BlockResizeCommand(b, ghost->AdjustedLength(), command);
      }
    }

    const TrackReference& track_ref = ghost->GetAdjustedTrack();

    new TrackPlaceBlockCommand(parent()->timeline_node_->track_list(track_ref.type()),
                               track_ref.index(),
                               b,
                               ghost->GetAdjustedIn(),
                               command);
  }

  olive::undo_stack.pushIfHasChildren(command);
}

rational TimelineWidget::PointerTool::FrameValidateInternal(rational time_movement, const QVector<TimelineViewGhostItem *>& ghosts)
{
  // Default behavior is to validate all movement and trimming
  time_movement = ValidateFrameMovement(time_movement, ghosts);
  time_movement = ValidateInTrimming(time_movement, ghosts, true);
  time_movement = ValidateOutTrimming(time_movement, ghosts, true);

  return time_movement;
}

void TimelineWidget::PointerTool::InitiateDrag(const TimelineCoordinate &mouse_pos)
{
  // Record where the drag started in timeline coordinates
  drag_start_ = mouse_pos;

  // Get the item that was clicked
  TimelineViewBlockItem* clicked_item = GetItemAtScenePos(drag_start_);

  // We only initiate a pointer drag if the user actually dragged an item, otherwise if they dragged on empty space
  // QGraphicsView default behavior would initiate a rubberband drag
  if (clicked_item != nullptr) {

    // Clear snap points
    snap_points_.clear();

    // Record where the drag started in timeline coordinates
    track_start_ = mouse_pos.GetTrack();

    // Determine whether we're trimming or moving based on the position of the cursor
    olive::timeline::MovementMode trim_mode = olive::timeline::kNone;

    // FIXME: Hardcoded number
    const int kTrimHandle = 10;

    qreal mouse_x = parent()->TimeToScene(mouse_pos.GetFrame());

    if (trimming_allowed_ && mouse_x < clicked_item->x() + kTrimHandle) {
      trim_mode = olive::timeline::kTrimIn;
    } else if (trimming_allowed_ && mouse_x > clicked_item->x() + clicked_item->rect().right() - kTrimHandle) {
      trim_mode = olive::timeline::kTrimOut;
    } else if (movement_allowed_) {
      // Some derived classes don't allow movement
      trim_mode = olive::timeline::kMove;
    }

    // Gaps can't be moved, only trimmed
    if (clicked_item->block()->type() == Block::kGap && trim_mode == olive::timeline::kMove) {
      trim_mode = olive::timeline::kNone;
    }

    // Make sure we can actually perform an action here
    if (trim_mode != olive::timeline::kNone) {
      InitiateGhosts(clicked_item, trim_mode, false);
    }
  }
}

void TimelineWidget::PointerTool::ProcessDrag(const TimelineCoordinate &mouse_pos)
{
  // Determine track movement
  const TrackReference& cursor_track = mouse_pos.GetTrack();
  int track_movement = 0;

  if (track_movement_allowed_) {
    track_movement = cursor_track.index() - track_start_.index();
  }

  // Determine frame movement
  rational time_movement = mouse_pos.GetFrame() - drag_start_.GetFrame();

  // Perform snapping if enabled (adjusts time_movement if it's close to any potential snap points)
  if (olive::core.snapping()) {
    SnapPoint(snap_points_, &time_movement);
  }

  // Validate movement (enforce all ghosts moving in legal ways)
  time_movement = FrameValidateInternal(time_movement, parent()->ghost_items_);
  track_movement = ValidateTrackMovement(track_movement, parent()->ghost_items_);

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
      // Also, we only move the clips on the same track type that the drag started from
      if (ghost->Track().type() == drag_track_type_) {
        ghost->SetTrackAdjustment(track_movement);
      }

      const TrackReference& track = ghost->GetAdjustedTrack();
      ghost->SetYCoords(parent()->GetTrackY(track), parent()->GetTrackHeight(track));
      break;
    }
    }
  }

  // Show tooltip
  // Generate tooltip (showing earliest in point of imported clip)
  int64_t earliest_timestamp = olive::time_to_timestamp(time_movement, parent()->timebase());
  QString tooltip_text = olive::timestamp_to_timecode(earliest_timestamp,
                                                      parent()->timebase(),
                                                      olive::CurrentTimecodeDisplay(),
                                                      true);
  QToolTip::showText(QCursor::pos(),
                     tooltip_text,
                     parent());
}

void TimelineWidget::PointerTool::InitiateGhosts(TimelineViewBlockItem* clicked_item,
                                               olive::timeline::MovementMode trim_mode,
                                               bool allow_gap_trimming)
{
  // Convert selected items list to clips list
  QList<TimelineViewBlockItem*> clips = parent()->GetSelectedBlocks();

  // If trimming multiple clips, we only trim the earliest in each track (trimming in) or the latest in each track
  // (trimming out). If the current clip is NOT one of these, we only trim it.
  bool multitrim_enabled = true;

  // Determine if the clicked item is the earliest/latest in the track for in/out trimming respectively
  if (trim_mode == olive::timeline::kTrimIn
      || trim_mode == olive::timeline::kTrimOut) {
    multitrim_enabled = IsClipTrimmable(clicked_item, clips, trim_mode);
  }

  // For each selected item, create a "ghost", a visual representation of the action before it gets performed
  foreach (TimelineViewBlockItem* clip_item, clips) {
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
      Block* block = clip_item->block();
      olive::timeline::MovementMode block_mode = trim_mode;

      if (block->type() == Block::kGap && !allow_gap_trimming) {
        if (trim_mode == olive::timeline::kTrimIn) {
          // Trim the previous clip's out point instead
          block = block->previous();
        } else {
          // Assume kTrimOut
          block = block->next();
        }
        block_mode = FlipTrimMode(trim_mode);
      }

      if (block != nullptr) {
        AddGhostFromBlock(block, clip_item->Track(), block_mode);
      }
    }
  }
}

TimelineViewGhostItem* TimelineWidget::PointerTool::AddGhostFromBlock(Block* block, const TrackReference& track, olive::timeline::MovementMode mode)
{
  TimelineViewGhostItem* ghost = TimelineViewGhostItem::FromBlock(block,
                                                                  track,
                                                                  parent()->GetTrackY(track),
                                                                  parent()->GetTrackHeight(track));

  AddGhostInternal(ghost, mode);

  return ghost;
}

TimelineViewGhostItem* TimelineWidget::PointerTool::AddGhostFromNull(const rational &in, const rational &out, const TrackReference& track, olive::timeline::MovementMode mode)
{
  TimelineViewGhostItem* ghost = new TimelineViewGhostItem();

  ghost->SetIn(in);
  ghost->SetOut(out);
  ghost->SetTrack(track);
  ghost->SetYCoords(parent()->GetTrackY(track), parent()->GetTrackHeight(track));

  AddGhostInternal(ghost, mode);

  return ghost;
}

void TimelineWidget::PointerTool::AddGhostInternal(TimelineViewGhostItem* ghost, olive::timeline::MovementMode mode)
{
  ghost->SetMode(mode);

  // Prepare snap points (optimizes snapping for later)
  switch (mode) {
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

  parent()->AddGhost(ghost);
}

bool TimelineWidget::PointerTool::IsClipTrimmable(TimelineViewBlockItem* clip,
                                                const QList<TimelineViewBlockItem*>& items,
                                                const olive::timeline::MovementMode& mode)
{
  foreach (TimelineViewBlockItem* compare, items) {
    if (clip->Track() == compare->Track()
        && clip != compare
        && ((compare->block()->in() < clip->block()->in() && mode == olive::timeline::kTrimIn)
            || (compare->block()->out() > clip->block()->out() && mode == olive::timeline::kTrimOut))) {
      return false;
    }
  }

  return true;
}

rational TimelineWidget::PointerTool::ValidateInTrimming(rational movement,
                                                       const QVector<TimelineViewGhostItem *> ghosts,
                                                       bool prevent_overwriting)
{
  foreach (TimelineViewGhostItem* ghost, ghosts) {
    if (ghost->mode() != olive::timeline::kTrimIn) {
      continue;
    }

    Block* block = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    // Determine the earliest in point this block could have
    rational earliest_in = qMax(rational(0), block->in() - block->media_in());

    if (prevent_overwriting) {
      // Look for a Block in the way
      Block* prev = block->previous();
      while (prev != nullptr) {
        if (prev->type() == Block::kClip) {
          earliest_in = qMax(earliest_in, prev->out());
          break;
        }
        prev = prev->previous();
      }
    }

    // Determine the latest point this block could have
    rational latest_in = ghost->Out();

    if (!ghost->CanHaveZeroLength()) {
      latest_in -= parent()->timebase();
    }

    // Clamp adjusted value between the earliest and latest values
    rational adjusted = ghost->In() + movement;
    rational clamped = clamp(adjusted, earliest_in, latest_in);

    if (clamped != adjusted) {
      movement = clamped - ghost->In();
    }
  }

  return movement;
}

rational TimelineWidget::PointerTool::ValidateOutTrimming(rational movement,
                                                        const QVector<TimelineViewGhostItem *> ghosts,
                                                        bool prevent_overwriting)
{
  foreach (TimelineViewGhostItem* ghost, ghosts) {
    if (ghost->mode() != olive::timeline::kTrimOut) {
      continue;
    }

    Block* block = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    // Determine earliest and latest out points
    rational earliest_out = ghost->In();

    if (!ghost->CanHaveZeroLength()) {
      earliest_out += parent()->timebase();
    }

    rational latest_out = RATIONAL_MAX;

    if (prevent_overwriting) {
      // Determine if there's a block in the way
      Block* next = block->next();
      while (next != nullptr) {
        if (next->type() == Block::kClip) {
          latest_out = qMin(latest_out, next->in());
          break;
        }
        next = next->next();
      }
    }

    // Clamp adjusted value between the earliest and latest values
    rational adjusted = ghost->Out() + movement;
    rational clamped = clamp(adjusted, earliest_out, latest_out);

    if (clamped != adjusted) {
      movement = clamped - ghost->Out();
    }
  }

  return movement;
}
