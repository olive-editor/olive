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
#include "common/qtutils.h"
#include "common/range.h"
#include "common/timecodefunctions.h"
#include "config/config.h"
#include "core.h"
#include "node/block/gap/gap.h"
#include "node/block/transition/transition.h"
#include "widget/nodeview/nodeviewundo.h"

OLIVE_NAMESPACE_ENTER

TimelineWidget::PointerTool::PointerTool(TimelineWidget *parent) :
  Tool(parent),
  movement_allowed_(true),
  trimming_allowed_(true),
  track_movement_allowed_(true),
  gap_trimming_allowed_(false),
  rubberband_selecting_(false)
{
}

void TimelineWidget::PointerTool::MousePress(TimelineViewMouseEvent *event)
{
  // Determine if item clicked on is selectable
  clicked_item_ = GetItemAtScenePos(event->GetCoordinates());

  bool selectable_item = (clicked_item_
                          && clicked_item_->flags() & QGraphicsItem::ItemIsSelectable
                          && !parent()->GetTrackFromReference(clicked_item_->Track())->IsLocked());

  if (selectable_item) {
    // Cache the clip's type for use later
    drag_track_type_ = clicked_item_->Track().type();

    // If we haven't started dragging yet, we'll initiate a drag here
    // Record where the drag started in timeline coordinates
    drag_start_ = event->GetCoordinates();

    // Determine whether we're trimming or moving based on the position of the cursor
    drag_movement_mode_ = IsCursorInTrimHandle(clicked_item_,
                                               event->GetSceneX());

    // If we're not in a trim mode, we must be in a move mode (provided the tool allows movement and
    // the block is not a gap)
    if (drag_movement_mode_ == Timeline::kNone
        && movement_allowed_
        && clicked_item_->block()->type() != Block::kGap) {
      drag_movement_mode_ = Timeline::kMove;
    }

    // If this item is already selected, no further selection needs to be made
    if (clicked_item_->isSelected()) {

      // If shift is held, deselect it
      if (event->GetModifiers() & Qt::ShiftModifier) {
        clicked_item_->setSelected(false);

        // If not holding alt, deselect all links as well
        if (!(event->GetModifiers() & Qt::AltModifier)) {
          parent()->SetBlockLinksSelected(clicked_item_->block(), false);
        }
      }

      return;
    }
  }

  // If not holding shift, deselect all clips
  if (!(event->GetModifiers() & Qt::ShiftModifier)) {
    parent()->DeselectAll();
  }

  if (selectable_item) {
    // Select this item
    clicked_item_->setSelected(true);

    // If not holding alt, select all links as well
    if (!(event->GetModifiers() & Qt::AltModifier)) {
      parent()->SetBlockLinksSelected(clicked_item_->block(), true);
    }
  } else if (event->GetButton() == Qt::LeftButton) {
    // Start rubberband drag
    parent()->StartRubberBandSelect(true, !(event->GetModifiers() & Qt::AltModifier));

    rubberband_selecting_ = true;
  }
}

void TimelineWidget::PointerTool::MouseMove(TimelineViewMouseEvent *event)
{
  if (rubberband_selecting_) {
    // Process rubberband select
    parent()->MoveRubberBandSelect(true, !(event->GetModifiers() & Qt::AltModifier));
  } else {
    // Process drag
    if (!dragging_) {

      // Now that the cursor has moved, we will assume the intention is to drag

      // Clear snap points
      snap_points_.clear();

      // If we're performing an action, we can initiate ghosts
      if (drag_movement_mode_ != Timeline::kNone) {
        InitiateDrag(clicked_item_, drag_movement_mode_);
      }

      // Set dragging to true here so no matter what, the drag isn't re-initiated until it's completed
      dragging_ = true;

    }

    if (dragging_ && !parent()->ghost_items_.isEmpty()) {

      // We're already dragging AND we have ghosts to work with
      ProcessDrag(event->GetCoordinates());

    }
  }
}

void TimelineWidget::PointerTool::MouseRelease(TimelineViewMouseEvent *event)
{
  if (rubberband_selecting_) {
    // Finish rubberband select
    parent()->EndRubberBandSelect(true, !(event->GetModifiers() & Qt::AltModifier));
    rubberband_selecting_ = false;
    return;
  }

  if (dragging_) {
    // If we were dragging, process the end of the drag
    if (!parent()->ghost_items_.isEmpty()) {
      FinishDrag(event);
    }

    // Clean up
    parent()->ClearGhosts();
    snap_points_.clear();

    dragging_ = false;
  }
}

void TimelineWidget::PointerTool::HoverMove(TimelineViewMouseEvent *event)
{
  if (trimming_allowed_) {
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
  } else {
    parent()->unsetCursor();
  }
}

void TimelineWidget::PointerTool::InitiateDragInternal(TimelineViewBlockItem *clicked_item,
                                                       Timeline::MovementMode trim_mode,
                                                       GhostMode pointer_mode,
                                                       bool trim_overwrite_allowed)
{
  // Get list of selected blocks
  QList<TimelineViewBlockItem*> clips = parent()->GetSelectedBlocks();

  if (trim_mode == Timeline::kMove) {

    // Each block type has different behavior, so we determine the type of the block that was
    // clicked and filter out any others.
    Block::Type clicked_block_type = clicked_item->block()->type();

    // Gaps are not allowed to move, and since we only allow moving one block type at a time,
    // dragging a gap is a no-op
    if (clicked_block_type == Block::kGap) {
      return;
    }

    // Create ghosts for moving
    foreach (TimelineViewBlockItem* clip_item, clips) {
      Block* block = clip_item->block();

      if (block->type() == Block::kGap) {
        // Gaps cannot move, ignore this block
        continue;
      }

      if (clicked_block_type == Block::kTransition && block->type() != Block::kTransition) {
        // Transitions always slide rather than move, so if we clicked a transition, ignore any
        // non-transitions
        continue;
      }

      // Create ghost
      TimelineViewGhostItem* ghost = AddGhostFromBlock(block, clip_item->Track(),
                                                       trim_mode, trim_overwrite_allowed);

      if (clicked_block_type == Block::kTransition) {
        // Transition moves are always a slide
        ghost->setData(TimelineViewGhostItem::kPointerToolMode, kSlide);
      } else {
        // Set to default behavior
        ghost->setData(TimelineViewGhostItem::kPointerToolMode, pointer_mode);

        // Include transitions (if any)
        AddMovingTransitionsToClipGhost(block, clip_item->Track(), trim_mode, clips);
      }
    }

    // If we slid the blocks, we must process them as such
    if (clicked_block_type == Block::kTransition || pointer_mode == kSlide) {
      ProcessGhostsForSliding();
    }

  } else {

    // "Multi-trim" is trimming a clip on more than one track. Only the earliest (for in trimming)
    // or latest (for out trimming) clip on each track can be trimmed. Therefore, it's only enabled
    // if the clicked item is the earliest/latest on its track.
    bool multitrim_enabled = IsClipTrimmable(clicked_item, clips, trim_mode);

    // Create ghosts for trimming
    foreach (TimelineViewBlockItem* clip_item, clips) {
      if (clip_item != clicked_item
          && (!multitrim_enabled || !IsClipTrimmable(clip_item, clips, trim_mode))) {
        // Either multitrim is disabled or this clip is NOT the earliest/latest in its track. We
        // won't include it.
        continue;
      }

      Block* block = clip_item->block();
      Timeline::MovementMode block_mode = trim_mode;
      bool block_trim_overwrite_allowed = trim_overwrite_allowed;

      // Some tools interpret "gap trimming" as equivalent to resizing the adjacent block. In that
      // scenario, we include the adjacent block instead.
      if (block->type() == Block::kGap && !gap_trimming_allowed_) {
        block = (trim_mode == Timeline::kTrimIn) ? block->previous() : block->next();
        block_mode = FlipTrimMode(trim_mode);

        // If there's no adjacent block, do nothing here
        if (!block) {
          continue;
        }
      }

      // For transitions, we create a rolling edit with the attached clip
      if (block->type() == Block::kTransition) {
        TransitionBlock* transition = static_cast<TransitionBlock*>(block);
        TimelineViewGhostItem* g = nullptr;

        Block* previous = transition->previous();
        Block* next = transition->next();

        if (block_mode == Timeline::kTrimIn
            && previous
            && (previous == transition->connected_out_block() || previous->type() == Block::kGap)) {
          g = AddGhostFromBlock(previous, clip_item->Track(), Timeline::kTrimOut, true);
        } else if (block_mode == Timeline::kTrimOut
                   && next
                   && (next == transition->connected_in_block() || next->type() == Block::kGap)) {
          g = AddGhostFromBlock(next, clip_item->Track(), Timeline::kTrimIn, true);
        }

        if (g) {
          g->setData(TimelineViewGhostItem::kPointerToolMode, kRolling);
          block_trim_overwrite_allowed = true;
        }
      }

      // Create ghost for this block
      TimelineViewGhostItem* ghost = AddGhostFromBlock(block, clip_item->Track(), block_mode, block_trim_overwrite_allowed);

      if (block->type() == Block::kTransition) {

        // If this is a transition, set to rolling as above
        ghost->setData(TimelineViewGhostItem::kPointerToolMode, kRolling);

      } else {

        // For trimmed clips, we also "move" the transitions if any are attached
        if (AddMovingTransitionsToClipGhost(block, clip_item->Track(), trim_mode, clips)) {
          ghost->setData(TimelineViewGhostItem::kPointerToolMode, kSlide);
        } else {
          ghost->setData(TimelineViewGhostItem::kPointerToolMode, kPointer);
        }
      }
    }

    if (pointer_mode == kRolling) {
      ProcessGhostsForRolling();
    }
  }
}

void TimelineWidget::PointerTool::ProcessDrag(const TimelineCoordinate &mouse_pos)
{
  // Calculate track movement
  int track_movement = track_movement_allowed_
      ? mouse_pos.GetTrack().index() - drag_start_.GetTrack().index()
      : 0;

  // Determine frame movement
  rational time_movement = mouse_pos.GetFrame() - drag_start_.GetFrame();

  // Validate movement (enforce all ghosts moving in legal ways)
  time_movement = ValidateTimeMovement(time_movement);
  time_movement = ValidateInTrimming(time_movement);
  time_movement = ValidateOutTrimming(time_movement);

  // Perform snapping if enabled (adjusts time_movement if it's close to any potential snap points)
  if (Core::instance()->snapping()) {
    parent()->SnapPoint(snap_points_, &time_movement);

    time_movement = ValidateTimeMovement(time_movement);
    time_movement = ValidateInTrimming(time_movement);
    time_movement = ValidateOutTrimming(time_movement);
  }

  // Validate ghosts that are being moved (clips from other track types do NOT get moved)
  {
    QVector<TimelineViewGhostItem*> validate_track_ghosts = parent()->ghost_items_;
    for (int i=0;i<validate_track_ghosts.size();i++) {
      if (validate_track_ghosts.at(i)->Track().type() != drag_track_type_) {
        validate_track_ghosts.removeAt(i);
        i--;
      }
    }
    track_movement = ValidateTrackMovement(track_movement, validate_track_ghosts);
  }

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

        const TrackReference& track = ghost->GetAdjustedTrack();
        ghost->SetYCoords(parent()->GetTrackY(track), parent()->GetTrackHeight(track));
      }
      break;
    }
    }
  }

  // Regenerate tooltip and force it to update (otherwise the tooltip won't move as written in the
  // documentation, and could get in the way of the cursor)
  QToolTip::hideText();
  QToolTip::showText(QCursor::pos(),
                     Timecode::timestamp_to_timecode(Timecode::time_to_timestamp(time_movement, parent()->GetToolTipTimebase()),
                                                     parent()->GetToolTipTimebase(),
                                                     Core::instance()->GetTimecodeDisplay(),
                                                     true),
                     parent());
}

struct GhostBlockPair {
  TimelineViewGhostItem* ghost;
  Block* block;
};

void TimelineWidget::PointerTool::FinishDrag(TimelineViewMouseEvent *event)
{
  QList<GhostBlockPair> blocks_moving;
  QList<GhostBlockPair> blocks_sliding;
  QList<GhostBlockPair> blocks_trimming;

  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    if (ghost->HasBeenAdjusted()) {
      Block* b = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

      if (ghost->mode() == Timeline::kMove) {
        if (ghost->data(TimelineViewGhostItem::kPointerToolMode) == kSlide) {
          blocks_sliding.append({ghost, b});
        } else {
          blocks_moving.append({ghost, b});
        }
      } else if (Timeline::IsATrimMode(ghost->mode())) {
        blocks_trimming.append({ghost, b});
      }
    }
  }

  if (blocks_moving.isEmpty()
      && blocks_trimming.isEmpty()
      && blocks_sliding.isEmpty()) {
    // No blocks were adjusted, so nothing to do
    return;
  }

  // See if we're duplicated because ALT is held (only moved blocks can duplicate)
  bool duplicate_clips = (!blocks_moving.isEmpty() && event->GetModifiers() & Qt::AltModifier);
  bool inserting = (!blocks_moving.isEmpty() && event->GetModifiers() & Qt::ControlModifier);

  // Slide info
  QVector<TrackSlideCommand::BlockSlideInfo> slide_info;

  QUndoCommand* command = new QUndoCommand();

  foreach (const GhostBlockPair& p, blocks_trimming) {
    TimelineViewGhostItem* ghost = p.ghost;

    GhostMode m = static_cast<GhostMode>(ghost->data(TimelineViewGhostItem::kPointerToolMode).toInt());

    switch (m) {
    case kPointer:
    case kRolling:
      if (m != kRolling || ghost->mode() == drag_movement_mode()) {
        BlockTrimCommand* c = new BlockTrimCommand(parent()->GetTrackFromReference(ghost->GetAdjustedTrack()),
                                                   p.block,
                                                   ghost->AdjustedLength(),
                                                   ghost->mode(),
                                                   command);

        if (m == kRolling) {
          c->SetAllowNonGapTrimming(true);
        }
      }
      break;
    case kSlide:
      slide_info.append({parent()->GetTrackFromReference(ghost->Track()),
                         p.block,
                         ghost->mode(),
                         ghost->AdjustedLength(),
                         ghost->Length()});
      break;
    }
  }

  if (!blocks_moving.isEmpty()) {
    // If we're not duplicating, "remove" the clips and replace them with gaps
    if (!duplicate_clips) {
      QList<Block*> blocks_to_delete;

      foreach (const GhostBlockPair& p, blocks_moving) {
        blocks_to_delete.append(p.block);
      }

      parent()->ReplaceBlocksWithGaps(blocks_to_delete, false, command);
    }

    if (inserting) {
      // If we're inserting, ripple everything at the destination with gaps
      InsertGapsAtGhostDestination(command);
    }

    // Now we can re-add each clip
    foreach (const GhostBlockPair& p, blocks_moving) {
      Block* block = p.block;

      if (duplicate_clips) {
        // Duplicate rather than move
        Node* copy = block->copy();

        new NodeAddCommand(static_cast<NodeGraph*>(block->parent()),
                           copy,
                           command);

        new NodeCopyInputsCommand(block, copy, true, command);

        // Place the copy instead of the original block
        block = static_cast<Block*>(copy);
      }

      const TrackReference& track_ref = p.ghost->GetAdjustedTrack();
      new TrackPlaceBlockCommand(parent()->GetConnectedNode()->track_list(track_ref.type()),
                                 track_ref.index(),
                                 block,
                                 p.ghost->GetAdjustedIn(),
                                 command);
    }
  }

  if (!blocks_sliding.isEmpty()) {
    foreach (const GhostBlockPair& p, blocks_sliding) {
      slide_info.append({parent()->GetTrackFromReference(p.ghost->Track()),
                         p.block,
                         p.ghost->mode(),
                         p.ghost->GetAdjustedIn(),
                         p.ghost->In()});
    }
  }

  if (!slide_info.isEmpty()) {
    new TrackSlideCommand(slide_info, command);
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
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

void TimelineWidget::PointerTool::InitiateDrag(TimelineViewBlockItem* clicked_item,
                                               Timeline::MovementMode trim_mode)
{
  InitiateDragInternal(clicked_item, trim_mode, kPointer, false);
}

TimelineViewGhostItem* TimelineWidget::PointerTool::AddGhostFromBlock(Block* block, const TrackReference& track, Timeline::MovementMode mode, bool trim_overwrite_allowed)
{
  TimelineViewGhostItem* ghost = TimelineViewGhostItem::FromBlock(block,
                                                                  track,
                                                                  parent()->GetTrackY(track),
                                                                  parent()->GetTrackHeight(track));

  AddGhostInternal(ghost, mode, trim_overwrite_allowed);

  return ghost;
}

TimelineViewGhostItem* TimelineWidget::PointerTool::AddGhostFromNull(const rational &in, const rational &out, const TrackReference& track, Timeline::MovementMode mode, bool trim_overwrite_allowed)
{
  TimelineViewGhostItem* ghost = new TimelineViewGhostItem();

  ghost->SetIn(in);
  ghost->SetOut(out);
  ghost->SetTrack(track);
  ghost->SetYCoords(parent()->GetTrackY(track), parent()->GetTrackHeight(track));

  AddGhostInternal(ghost, mode, trim_overwrite_allowed);

  return ghost;
}

void TimelineWidget::PointerTool::AddGhostInternal(TimelineViewGhostItem* ghost, Timeline::MovementMode mode, bool trim_overwrite_allowed)
{
  ghost->SetMode(mode);
  ghost->setData(TimelineViewGhostItem::kTrimOverwriteAllowed, trim_overwrite_allowed);

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

struct TrackBlockListPair {
  TrackReference track;
  QList<Block*> blocks;
};

void TimelineWidget::PointerTool::ProcessGhostsForSliding()
{
  // Sort blocks into tracks
  QList<TrackBlockListPair> blocks_per_track;
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* b = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));
    bool found = false;

    for (int i=0;i<blocks_per_track.size();i++) {
      if (blocks_per_track.at(i).track == ghost->Track()) {
        blocks_per_track[i].blocks.append(b);
        found = true;
        break;
      }
    }

    if (!found) {
      blocks_per_track.append({ghost->Track(), {b}});
    }
  }

  // Make contiguous runs of blocks per each track
  foreach (const TrackBlockListPair& p, blocks_per_track) {
    // Blocks must be merged if any are non-adjacent
    const TrackReference& track = p.track;
    const QList<Block*>& blocks = p.blocks;

    Block* earliest_block = blocks.first();
    Block* latest_block = blocks.first();

    // Find the earliest and latest selected blocks
    for (int j=1;j<blocks.size();j++) {
      Block* compare = blocks.at(j);

      if (compare->in() < earliest_block->in()) {
        earliest_block = compare;
      }

      if (compare->in() > latest_block->in()) {
        latest_block = compare;
      }
    }

    // Add any blocks between these blocks that aren't already in the list
    if (earliest_block != latest_block) {
      Block* b = earliest_block;
      while ((b = b->next()) != latest_block) {
        if (!blocks.contains(b)) {
          TimelineViewGhostItem* g = AddGhostFromBlock(b, track, Timeline::kMove, true);
          g->setData(TimelineViewGhostItem::kPointerToolMode, kSlide);
        }
      }
    }

    // Add surrounding blocks that will be trimming instead of moving
    if (earliest_block->previous()) {
      TimelineViewGhostItem* g = AddGhostFromBlock(earliest_block->previous(), track, Timeline::kTrimOut, true);
      g->setData(TimelineViewGhostItem::kPointerToolMode, kSlide);
    }

    if (latest_block->next()) {
      TimelineViewGhostItem* g = AddGhostFromBlock(latest_block->next(), track, Timeline::kTrimIn, true);
      g->setData(TimelineViewGhostItem::kPointerToolMode, kSlide);
    }
  }
}

void TimelineWidget::PointerTool::ProcessGhostsForRolling()
{
  // For each ghost, we make an equivalent Ghost on the next/previous block
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    Block* ghost_block = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    if (ghost->mode() == Timeline::kTrimIn && ghost_block->previous()) {
      // Add an extra Ghost for the previous block
      AddGhostFromBlock(ghost_block->previous(), ghost->Track(), Timeline::kTrimOut, true);
    } else if (ghost->mode() == Timeline::kTrimOut && ghost_block->next()) {
      AddGhostFromBlock(ghost_block->next(), ghost->Track(), Timeline::kTrimIn, true);
    }
  }
}

bool TimelineWidget::PointerTool::AddMovingTransitionsToClipGhost(Block* block,
                                                                  const TrackReference& track,
                                                                  Timeline::MovementMode movement,
                                                                  const QList<TimelineViewBlockItem*>& selected_items)
{
  // Assume block is a clip and see if it has any transitions
  TransitionBlock* transitions[2];

  if (movement == Timeline::kMove || movement == Timeline::kTrimOut) {
    transitions[0] = TransitionBlock::GetBlockOutTransition(block);
  } else {
    transitions[0] = nullptr;
  }

  if (movement == Timeline::kMove || movement == Timeline::kTrimIn) {
    transitions[1] = TransitionBlock::GetBlockInTransition(block);
  } else {
    transitions[1] = nullptr;
  }

  bool ret = false;

  for (int i=0;i<2;i++) {
    if (!transitions[i]) {
      continue;
    }

    bool found = false;

    foreach (TimelineViewBlockItem* item, selected_items) {
      if (item->block() == transitions[i]) {
        // Do nothing
        found = true;
        break;
      }
    }

    if (!found) {
      TimelineViewGhostItem* transition_ghost = AddGhostFromBlock(transitions[i], track,
                                                                  Timeline::kMove, false);
      transition_ghost->setData(TimelineViewGhostItem::kPointerToolMode, kPointer);

      ret = true;
    }
  }

  return ret;
}

rational TimelineWidget::PointerTool::ValidateInTrimming(rational movement)
{
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    if (ghost->mode() != Timeline::kTrimIn) {
      continue;
    }

    Block* block = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    rational earliest_in = RATIONAL_MIN;
    rational latest_in = ghost->Out();

    if (!ghost->CanHaveZeroLength()) {
      latest_in -= parent()->timebase();
    }

    if (block) {
      if (!ghost->data(TimelineViewGhostItem::kTrimOverwriteAllowed).toBool()) {
        // Look for a Block in the way
        Block* prev = block->previous();
        while (prev != nullptr) {
          if (prev->type() == Block::kClip) {
            earliest_in = qMax(earliest_in, prev->out());
            break;
          }
          prev = prev->previous();
        }

        // Limit in point at 0 on the timeline
        earliest_in = qMax(rational(), earliest_in);
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

rational TimelineWidget::PointerTool::ValidateOutTrimming(rational movement)
{
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
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

    // Ripple tool creates block-less ghosts and creates gaps with them later
    if (block) {
      if (!ghost->data(TimelineViewGhostItem::kTrimOverwriteAllowed).toBool()) {
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

OLIVE_NAMESPACE_EXIT
