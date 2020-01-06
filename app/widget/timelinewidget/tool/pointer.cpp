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
#include "common/qtversionabstraction.h"
#include "common/range.h"
#include "common/timecodefunctions.h"
#include "config/config.h"
#include "core.h"
#include "node/block/gap/gap.h"
#include "node/block/transition/transition.h"
#include "widget/nodeview/nodeviewundo.h"

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
    InitiateDrag(event);

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

void TimelineWidget::PointerTool::HoverMove(TimelineViewMouseEvent *event)
{
  // No dragging, but we still want to process cursors
  TimelineViewBlockItem* block_at_cursor = GetItemAtScenePos(event->GetCoordinates());

  if (block_at_cursor) {
    switch (IsCursorInTrimHandle(block_at_cursor, event->GetSceneX())) {
    case Timeline::kTrimIn:
      parent()->setCursor(Qt::SizeHorCursor);
      break;
    case Timeline::kTrimOut:
      parent()->setCursor(Qt::SizeHorCursor);
      break;
    default:
      parent()->unsetCursor();
    }
  } else {
    parent()->unsetCursor();
  }
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
  QUndoCommand* command = new QUndoCommand();

  QList<Block*> blocks_to_temp_remove;
  QList<TrackReference> tracks_affected;
  QList<TimelineViewGhostItem*> ignore_ghosts;

  bool duplicate_clips = (event->GetModifiers() & Qt::AltModifier);

  // Since all the ghosts will be leaving their old position in some way, we replace all of them with gaps here so the
  // entire timeline isn't disrupted in the process
  for (int i=0;i<parent()->ghost_items_.size();i++) {
    TimelineViewGhostItem* ghost = parent()->ghost_items_.at(i);

    // If the ghost has not been adjusted nothing needs to be done
    if (!ghost->HasBeenAdjusted()) {
      ignore_ghosts.append(ghost);
      continue;
    }

    Block* b = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    if (!duplicate_clips || ghost->mode() != Timeline::kMove || b->type() == Block::kTransition) {
      // If we're duplicating (user is holding ALT), no need to remove the original clip. However if the ghost was
      // trimmed, it can't be duplicated.
      blocks_to_temp_remove.append(b);
    }

    if (!tracks_affected.contains(ghost->Track())) {
      tracks_affected.append(ghost->Track());
    }

    if (!tracks_affected.contains(ghost->GetAdjustedTrack())) {
      tracks_affected.append(ghost->GetAdjustedTrack());
    }
  }

  // If there are any blocks to remove, remove them
  parent()->DeleteSelectedInternal(blocks_to_temp_remove, false, false, command);

  // Now we place the clips back in the timeline where the user moved them. It's legal for them to overwrite parts or
  // all of the gaps we inserted earlier
  for (int i=0;i<parent()->ghost_items_.size();i++) {
    TimelineViewGhostItem* ghost = parent()->ghost_items_.at(i);

    // If the ghost has not been adjusted nothing needs to be done
    if (ignore_ghosts.contains(ghost)) {
      continue;
    }

    const TrackReference& track_ref = ghost->GetAdjustedTrack();
    //TrackOutput* track = parent()->GetTrackFromReference(track_ref);

    Block* b = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    // Normal blocks work in conjunction with the gap made above
    if (Timeline::IsATrimMode(ghost->mode())) {
      // If we were trimming, we'll need to change the length

      // If we were trimming the in point, we'll need to adjust the media in too
      if (ghost->mode() == Timeline::kTrimIn) {
        new BlockResizeWithMediaInCommand(b, ghost->AdjustedLength(), command);
      } else {
        new BlockResizeCommand(b, ghost->AdjustedLength(), command);
      }
    } else if (duplicate_clips && ghost->mode() == Timeline::kMove && b->type() != Block::kTransition) {
      // Duplicate rather than move
      Node* copy = b->copy();

      new NodeAddCommand(static_cast<NodeGraph*>(b->parent()),
                         copy,
                         command);

      new NodeCopyInputsCommand(b, copy, true, command);

      // Place the copy instead of the original block
      b = static_cast<Block*>(copy);
    } else if (b->type() == Block::kTransition) {
      // If the block is a dual transition and we're moving it, the mid point should be moved
      TransitionBlock* transition = static_cast<TransitionBlock*>(b);

      if (transition->connected_in_block() && transition->connected_out_block()) {
        new BlockSetMediaInCommand(transition,
                                   transition->media_in() + ghost->InAdjustment(),
                                   command);
      }
    }

    if (b->type() == Block::kTransition && ghost->AdjustedLength() == 0) {
      // Remove transitions that have been reduced to zero length
      new NodeRemoveCommand(static_cast<NodeGraph*>(b->parent()),
                            {b},
                            command);
    } else {
      // Normal block placement
      new TrackPlaceBlockCommand(parent()->timeline_node_->track_list(track_ref.type()),
                                 track_ref.index(),
                                 b,
                                 ghost->GetAdjustedIn(),
                                 command);
    }
  }

  if (command->childCount() > 0) {
    foreach (const TrackReference& t, tracks_affected) {
      new TrackCleanGapsCommand(parent()->timeline_node_->track_list(t.type()),
                                t.index(),
                                command);
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

rational TimelineWidget::PointerTool::FrameValidateInternal(rational time_movement, const QVector<TimelineViewGhostItem *>& ghosts)
{
  // Default behavior is to validate all movement and trimming
  time_movement = ValidateFrameMovement(time_movement, ghosts);
  time_movement = ValidateInTrimming(time_movement, ghosts, true);
  time_movement = ValidateOutTrimming(time_movement, ghosts, true);

  return time_movement;
}

void TimelineWidget::PointerTool::InitiateDrag(TimelineViewMouseEvent *mouse_pos)
{
  // Record where the drag started in timeline coordinates
  drag_start_ = mouse_pos->GetCoordinates();

  // Get the item that was clicked
  TimelineViewBlockItem* clicked_item = GetItemAtScenePos(drag_start_);

  // We only initiate a pointer drag if the user actually dragged an item, otherwise if they dragged on empty space
  // QGraphicsView default behavior would initiate a rubberband drag
  if (clicked_item != nullptr) {

    // Clear snap points
    snap_points_.clear();

    // Record where the drag started in timeline coordinates
    track_start_ = mouse_pos->GetTrack();

    // Determine whether we're trimming or moving based on the position of the cursor
    Timeline::MovementMode trim_mode = IsCursorInTrimHandle(clicked_item, mouse_pos->GetSceneX());

    // Some derived classes don't allow movement
    if (trim_mode == Timeline::kNone && movement_allowed_) {
      trim_mode = Timeline::kMove;
    }

    // Gaps can't be moved, only trimmed
    if (clicked_item->block()->type() == Block::kGap && trim_mode == Timeline::kMove) {
      trim_mode = Timeline::kNone;
    }

    // Make sure we can actually perform an action here
    if (trim_mode != Timeline::kNone) {
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
  if (Core::instance()->snapping()) {
    SnapPoint(snap_points_, &time_movement);
  }

  // Validate movement (enforce all ghosts moving in legal ways)
  time_movement = FrameValidateInternal(time_movement, parent()->ghost_items_);
  track_movement = ValidateTrackMovement(track_movement, parent()->ghost_items_);

  // Perform movement
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    switch (ghost->mode()) {
    case Timeline::kNone:
      break;
    case Timeline::kTrimIn:
      ghost->SetInAdjustment(time_movement);
      break;
    case Timeline::kTrimOut:
      ghost->SetOutAdjustment(time_movement);
      break;
    case Timeline::kMove:
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
  int64_t earliest_timestamp = Timecode::time_to_timestamp(time_movement, parent()->timebase());
  QString tooltip_text = Timecode::timestamp_to_timecode(earliest_timestamp,
                                                         parent()->timebase(),
                                                         Timecode::CurrentDisplay(),
                                                         true);

  // Force tooltip to update (otherwise the tooltip won't move as written in the documentation, and could get in the way
  // of the cursor)
  QToolTip::hideText();
  QToolTip::showText(QCursor::pos(),
                     tooltip_text,
                     parent());
}

Timeline::MovementMode TimelineWidget::PointerTool::IsCursorInTrimHandle(TimelineViewBlockItem *block, qreal cursor_x)
{
  double kTrimHandle = QFontMetricsWidth(parent()->fontMetrics(), "H");

  // Block is too narrow, no trimming allowed
  if (block->rect().width() <= kTrimHandle * 2) {
    return Timeline::kNone;
  }

  if (trimming_allowed_ && cursor_x <= block->x() + kTrimHandle) {
    return Timeline::kTrimIn;
  } else if (trimming_allowed_ && cursor_x >= block->x() + block->rect().right() - kTrimHandle) {
    return Timeline::kTrimOut;
  } else {
    return Timeline::kNone;
  }
}

void TimelineWidget::PointerTool::InitiateGhosts(TimelineViewBlockItem* clicked_item,
                                                 Timeline::MovementMode trim_mode,
                                                 bool allow_gap_trimming)
{
  // Convert selected items list to clips list
  QList<TimelineViewBlockItem*> clips = parent()->GetSelectedBlocks();

  // If trimming multiple clips, we only trim the earliest in each track (trimming in) or the latest in each track
  // (trimming out). If the current clip is NOT one of these, we only trim it.
  bool multitrim_enabled = true;

  // Determine if the clicked item is the earliest/latest in the track for in/out trimming respectively
  if (Timeline::IsATrimMode(trim_mode)) {
    multitrim_enabled = IsClipTrimmable(clicked_item, clips, trim_mode);
  }

  // For each selected item, create a "ghost", a visual representation of the action before it gets performed
  foreach (TimelineViewBlockItem* clip_item, clips) {

    // Determine correct mode for ghost
    //
    // Movement is indiscriminate, all the ghosts can be set to do this, however trimming is limited to one block
    // PER TRACK

    bool include_this_clip = true;

    if (clip_item->block()->type() == Block::kGap
        && !Timeline::IsATrimMode(trim_mode)) {
      continue;
    }

    if (clip_item != clicked_item
        && (Timeline::IsATrimMode(trim_mode))) {
      include_this_clip = multitrim_enabled ? IsClipTrimmable(clip_item, clips, trim_mode) : false;
    }

    if (include_this_clip) {
      Block* block = clip_item->block();
      Timeline::MovementMode block_mode = trim_mode;

      // If we don't allow gap trimming, we automatically switch to the next/previous block to trim
      if (block->type() == Block::kGap && !allow_gap_trimming) {
        if (trim_mode == Timeline::kTrimIn) {
          // Trim the previous clip's out point instead
          block = block->previous();
        } else {
          // Assume kTrimOut
          block = block->next();
        }
        block_mode = FlipTrimMode(trim_mode);
      }

      if (block) {
        TimelineViewGhostItem* ghost = AddGhostFromBlock(block, clip_item->Track(), block_mode);

        if (block->type() == Block::kTransition) {
          TransitionBlock* transition = static_cast<TransitionBlock*>(block);

          bool transition_can_move_tracks = false;

          // Create a rolling effect with the attached block
          if (transition->connected_in_block()) {
            if (parent()->block_items_.value(transition->connected_in_block())->isSelected()) {
              // We'll be moving this item too, no need to create a ghost for it here
              transition_can_move_tracks = true;
            } else if (block_mode == Timeline::kTrimOut || block_mode == Timeline::kMove) {
              AddGhostFromBlock(transition->connected_in_block(), clip_item->Track(), Timeline::kTrimIn);
            }
          }

          if (transition->connected_out_block()) {
            if (parent()->block_items_.value(transition->connected_in_block())->isSelected()) {
                // We'll be moving this item too, no need to create a ghost for it here
              transition_can_move_tracks = true;
            } else if (block_mode == Timeline::kTrimIn || block_mode == Timeline::kMove) {
              AddGhostFromBlock(transition->connected_out_block(), clip_item->Track(), Timeline::kTrimOut);
            }
          }

          ghost->SetCanMoveTracks(transition_can_move_tracks);
        }
      }
    }
  }
}

TimelineViewGhostItem* TimelineWidget::PointerTool::AddGhostFromBlock(Block* block, const TrackReference& track, Timeline::MovementMode mode)
{
  TimelineViewGhostItem* ghost = TimelineViewGhostItem::FromBlock(block,
                                                                  track,
                                                                  parent()->GetTrackY(track),
                                                                  parent()->GetTrackHeight(track));

  AddGhostInternal(ghost, mode);

  return ghost;
}

TimelineViewGhostItem* TimelineWidget::PointerTool::AddGhostFromNull(const rational &in, const rational &out, const TrackReference& track, Timeline::MovementMode mode)
{
  TimelineViewGhostItem* ghost = new TimelineViewGhostItem();

  ghost->SetIn(in);
  ghost->SetOut(out);
  ghost->SetTrack(track);
  ghost->SetYCoords(parent()->GetTrackY(track), parent()->GetTrackHeight(track));

  AddGhostInternal(ghost, mode);

  return ghost;
}

void TimelineWidget::PointerTool::AddGhostInternal(TimelineViewGhostItem* ghost, Timeline::MovementMode mode)
{
  ghost->SetMode(mode);

  // Prepare snap points (optimizes snapping for later)
  switch (mode) {
  case Timeline::kMove:
    snap_points_.append(ghost->In());
    snap_points_.append(ghost->Out());
    break;
  case Timeline::kTrimIn:
    snap_points_.append(ghost->In());
    break;
  case Timeline::kTrimOut:
    snap_points_.append(ghost->Out());
    break;
  default:
    break;
  }

  parent()->AddGhost(ghost);
}

bool TimelineWidget::PointerTool::IsClipTrimmable(TimelineViewBlockItem* clip,
                                                  const QList<TimelineViewBlockItem*>& items,
                                                  const Timeline::MovementMode& mode)
{
  foreach (TimelineViewBlockItem* compare, items) {
    if (clip->Track() == compare->Track()
        && clip != compare
        && ((compare->block()->in() < clip->block()->in() && mode == Timeline::kTrimIn)
            || (compare->block()->out() > clip->block()->out() && mode == Timeline::kTrimOut))) {
      return false;
    }
  }

  return true;
}

rational GetEarliestPointForClip(Block* block)
{
  return qMax(rational(0), block->in() - block->media_in());
}

rational TimelineWidget::PointerTool::ValidateInTrimming(rational movement,
                                                         const QVector<TimelineViewGhostItem *> ghosts,
                                                         bool prevent_overwriting)
{
  foreach (TimelineViewGhostItem* ghost, ghosts) {
    if (ghost->mode() != Timeline::kTrimIn) {
      continue;
    }

    Block* block = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    rational earliest_in = RATIONAL_MIN;
    rational latest_in = ghost->Out();

    if (block->type() == Block::kTransition) {
      // For transitions, validate with the attached block
      TransitionBlock* transition = static_cast<TransitionBlock*>(block);

      if (transition->connected_in_block() && transition->connected_out_block()) {
        // Here, we try to get the latest earliest point for both the in and out blocks, we do in here and out will
        // be calculated later
        earliest_in = GetEarliestPointForClip(transition->connected_in_block());

        // We set the block to the out block since that will be before the in block and will be the one we use to
        // prevent overwriting since we're trimming the in side of this transition
        block = transition->connected_out_block();

        latest_in = transition->in() + transition->out_offset();
      } else {
        // Use whatever block is attached
        block = transition->connected_in_block() ? transition->connected_in_block() : transition->connected_out_block();
      }
    }

    earliest_in = qMax(earliest_in, GetEarliestPointForClip(block));

    if (!ghost->CanHaveZeroLength()) {
      latest_in -= parent()->timebase();
    }

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
    if (ghost->mode() != Timeline::kTrimOut) {
      continue;
    }

    Block* block = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    // Determine earliest and latest out points
    rational earliest_out = ghost->In();

    if (!ghost->CanHaveZeroLength()) {
      earliest_out += parent()->timebase();
    }

    rational latest_out = RATIONAL_MAX;

    if (block->type() == Block::kTransition) {
      // For transitions, validate with the attached block
      TransitionBlock* transition = static_cast<TransitionBlock*>(block);

      if (transition->connected_in_block() && transition->connected_out_block()) {
        // We set the block to the out block since that will be before the in block and will be the one we use to
        // prevent overwriting since we're trimming the in side of this transition

        // FIXME: At some point we may add some better logic to `latest_out` akin to the logic in ValidateInTrimming
        //        which is why this hasn't yet been collapsed into the ternary below.
        block = transition->connected_in_block();

        earliest_out = transition->out() - transition->in_offset();
      } else {
        block = transition->connected_in_block() ? transition->connected_in_block() : transition->connected_out_block();
      }
    }

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
