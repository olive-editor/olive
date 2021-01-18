/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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
#include "pointer.h"
#include "widget/nodeview/nodeviewundo.h"

namespace olive {

PointerTool::PointerTool(TimelineWidget *parent) :
  TimelineTool(parent),
  movement_allowed_(true),
  trimming_allowed_(true),
  track_movement_allowed_(true),
  gap_trimming_allowed_(false),
  rubberband_selecting_(false)
{
}

void PointerTool::MousePress(TimelineViewMouseEvent *event)
{
  const Track::Reference& track_ref = event->GetTrack();

  // Determine if item clicked on is selectable
  clicked_item_ = parent()->GetItemAtScenePos(event->GetCoordinates());

  can_rubberband_select_ = false;

  bool selectable_item = (clicked_item_
                          && !parent()->GetTrackFromReference(track_ref)->IsLocked());

  if (selectable_item) {
    // Cache the clip's type for use later
    drag_track_type_ = track_ref.type();

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
        && clicked_item_->type() != Block::kGap) {
      drag_movement_mode_ = Timeline::kMove;
    }

    // If this item is already selected, no further selection needs to be made
    if (parent()->IsBlockSelected(clicked_item_)) {

      // Collect item deselections
      QVector<Block*> deselected_blocks;

      // If shift is held, deselect it
      if (event->GetModifiers() & Qt::ShiftModifier) {
        parent()->RemoveSelection(clicked_item_);
        deselected_blocks.append(clicked_item_);

        // If not holding alt, deselect all links as well
        if (!(event->GetModifiers() & Qt::AltModifier)) {
          parent()->SetBlockLinksSelected(clicked_item_, false);
          deselected_blocks.append(clicked_item_->linked_clips());
        }
      }

      parent()->SignalDeselectedBlocks(deselected_blocks);

      return;
    }
  }

  // If not holding shift, deselect all clips
  if (!(event->GetModifiers() & Qt::ShiftModifier)) {
    parent()->DeselectAll();
  }

  if (selectable_item) {

    // Collect item selections
    QVector<Block*> selected_blocks;

    // Select this item
    parent()->AddSelection(clicked_item_);
    selected_blocks.append(clicked_item_);

    // If not holding alt, select all links as well
    if (!(event->GetModifiers() & Qt::AltModifier)) {
      parent()->SetBlockLinksSelected(clicked_item_, true);
      selected_blocks.append(clicked_item_->linked_clips());
    }

    parent()->SignalSelectedBlocks(selected_blocks);

  }

  can_rubberband_select_ =  (event->GetButton() == Qt::LeftButton                              // Only rubberband select from the primary mouse button
                             && (!selectable_item || drag_movement_mode_ == Timeline::kNone)); // And if no item was selected OR the item isn't draggable

  if (can_rubberband_select_) {
    drag_global_start_ = QCursor::pos();
  }
}

void PointerTool::MouseMove(TimelineViewMouseEvent *event)
{
  if (can_rubberband_select_) {

    if (!rubberband_selecting_) {

      // If we clicked an item but are rubberband selecting anyway, deselect it now
      if (clicked_item_) {
        parent()->RemoveSelection(clicked_item_);
        parent()->SignalDeselectedBlocks({clicked_item_});
        clicked_item_ = nullptr;
      }

      parent()->StartRubberBandSelect(drag_global_start_);

      rubberband_selecting_ = true;

    }

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

    if (dragging_ && !parent()->GetGhostItems().isEmpty()) {

      // We're already dragging AND we have ghosts to work with
      ProcessDrag(event->GetCoordinates());

    }
  }
}

void PointerTool::MouseRelease(TimelineViewMouseEvent *event)
{
  if (rubberband_selecting_) {
    // Finish rubberband select
    parent()->EndRubberBandSelect();
    rubberband_selecting_ = false;
    return;
  }

  if (dragging_) {
    // If we were dragging, process the end of the drag
    if (!parent()->GetGhostItems().isEmpty()) {
      FinishDrag(event);
    }

    // Clean up
    parent()->ClearGhosts();
    snap_points_.clear();

    dragging_ = false;
  }
}

void PointerTool::HoverMove(TimelineViewMouseEvent *event)
{
  if (trimming_allowed_) {
    // No dragging, but we still want to process cursors
    Block* block_at_cursor = parent()->GetItemAtScenePos(event->GetCoordinates());

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

void SetGhostToSlideMode(TimelineViewGhostItem* g)
{
  g->SetCanMoveTracks(false);
  g->SetData(TimelineViewGhostItem::kGhostIsSliding, true);
}

void PointerTool::InitiateDragInternal(Block *clicked_item,
                                       Timeline::MovementMode trim_mode,
                                       bool dont_roll_trims,
                                       bool allow_nongap_rolling,
                                       bool slide_instead_of_moving)
{
  // Get list of selected blocks
  QVector<Block*> clips = parent()->GetSelectedBlocks();

  if (trim_mode == Timeline::kMove) {

    // Each block type has different behavior, so we determine the type of the block that was
    // clicked and filter out any others.
    Block::Type clicked_block_type = clicked_item->type();

    // Gaps are not allowed to move, and since we only allow moving one block type at a time,
    // dragging a gap is a no-op
    if (clicked_block_type == Block::kGap) {
      return;
    }

    // Determine if this move is a slide, which is determined by either
    bool clips_are_sliding = (slide_instead_of_moving || clicked_block_type == Block::kTransition);

    if (clips_are_sliding) {
      // This is a slide. What we do here is move clips within their own track, between the clips
      // that they're already next to. We don't allow changing tracks or changing the order of
      // blocks.
      //
      // For slides to be legal, we make all blocks "contiguous". This means that only one series
      // of blocks can move at a time and prevents.

      QHash<Track*, Block*> earliest_block_on_track;
      QHash<Track*, Block*> latest_block_on_track;

      foreach (Block* this_block, clips) {
        Block* current_earliest = earliest_block_on_track.value(this_block->track(), nullptr);
        if (!current_earliest || this_block->in() < current_earliest->in()) {
          earliest_block_on_track.insert(this_block->track(), this_block);
        }

        Block* current_latest = latest_block_on_track.value(this_block->track(), nullptr);
        if (!current_latest || this_block->out() > current_earliest->out()) {
          latest_block_on_track.insert(this_block->track(), this_block);
        }
      }

      for (auto i=earliest_block_on_track.constBegin(); i!=earliest_block_on_track.constEnd(); i++) {
        // Make a contiguous stream
        Track* track = i.key();
        Block* earliest = i.value();
        Block* latest = latest_block_on_track.value(i.key());

        // First we add the block that's out trimming, the one prior to the earliest
        TimelineViewGhostItem* earliest_ghost;
        if (earliest->previous()) {
          earliest_ghost = AddGhostFromBlock(earliest->previous(), Timeline::kTrimOut);
        } else {
          earliest_ghost = AddGhostFromNull(earliest->in(), earliest->in(), track->ToReference(), Timeline::kTrimOut);
        }
        SetGhostToSlideMode(earliest_ghost);

        // Then we add the block that's in trimming, the one after the latest
        if (latest->next()) {
          TimelineViewGhostItem* latest_ghost = AddGhostFromBlock(latest->next(), Timeline::kTrimIn);
          SetGhostToSlideMode(latest_ghost);
        }

        // Finally, we add all of the moving blocks in between
        Block* b = nullptr;
        do {
          // On first run-through, set to earliest only. From then on, set to the next of the last
          // in the loop.
          if (b) {
            b = b->next();
          } else {
            b = earliest;
          }

          TimelineViewGhostItem* between_ghost = AddGhostFromBlock(b, Timeline::kMove);
          SetGhostToSlideMode(between_ghost);
        } while (b != latest);
      }
    } else {
      // Prepare for a standard pointer move
      foreach (Block* block, clips) {
        if (block->type() == Block::kGap || block->type() == Block::kTransition) {
          // Gaps cannot move, and we handle transitions further down
          continue;
        }

        // Create ghost
        TimelineViewGhostItem* ghost = AddGhostFromBlock(block,
                                                         trim_mode);
        Q_UNUSED(ghost)

        // Add transitions if this has any
        TransitionBlock* opening_transition = block->in_transition();
        TransitionBlock* closing_transition = block->out_transition();

        if (opening_transition) {
          TimelineViewGhostItem* ot_ghost = AddGhostFromBlock(opening_transition,
                                                              trim_mode);
          Q_UNUSED(ot_ghost)
        }

        if (closing_transition) {
          TimelineViewGhostItem* cl_ghost = AddGhostFromBlock(closing_transition,
                                                              trim_mode);
          Q_UNUSED(cl_ghost)
        }
      }
    }

  } else {

    // "Multi-trim" is trimming a clip on more than one track. Only the earliest (for in trimming)
    // or latest (for out trimming) clip on each track can be trimmed. Therefore, it's only enabled
    // if the clicked item is the earliest/latest on its track.
    bool multitrim_enabled = IsClipTrimmable(clicked_item, clips, trim_mode);

    // Create ghosts for trimming
    foreach (Block* clip_item, clips) {
      if (clip_item != clicked_item
          && (!multitrim_enabled || !IsClipTrimmable(clip_item, clips, trim_mode))) {
        // Either multitrim is disabled or this clip is NOT the earliest/latest in its track. We
        // won't include it.
        continue;
      }

      Block* block = clip_item;

      // Create ghost for this block
      TimelineViewGhostItem* ghost = AddGhostFromBlock(block, trim_mode);

      // If this side of the clip has a transition, we treat it more like a slide for that
      // transition than a trim/roll
      bool treat_trim_as_slide = false;

      if (block->type() == Block::kClip) {
        // See if this clip has a transition attached, and move it with the trim if so
        TransitionBlock* connected_transition;

        // Get appropriate transition for the side of the clip
        if (trim_mode == Timeline::kTrimIn) {
          connected_transition = block->in_transition();
        } else {
          connected_transition = block->out_transition();
        }

        if (connected_transition) {
          // We found a transition, we'll make this a "slide" action
          TimelineViewGhostItem* transition_ghost = AddGhostFromBlock(connected_transition, Timeline::kMove);

          // This will in effect be a slide with the transition moving between two other blocks
          SetGhostToSlideMode(ghost);
          SetGhostToSlideMode(transition_ghost);
          treat_trim_as_slide = true;

          // Further processing will apply to this transition rather than the clip
          block = connected_transition;
        }
      }

      // Standard pointer trimming in reality is a "roll" edit with an adjacent gap (one that may
      // or may not exist already)
      if (!dont_roll_trims) {
        Block* adjacent = nullptr;

        // Determine which block is adjacent
        if (trim_mode == Timeline::kTrimIn) {
          adjacent = block->previous();
        } else {
          adjacent = block->next();
        }

        // See if we can roll the adjacent or if we'll need to create our own gap
        if (block->type() != Block::kGap
            && !allow_nongap_rolling && adjacent && adjacent->type() != Block::kGap
            && !(block->type() == Block::kTransition
                 && ((trim_mode == Timeline::kTrimIn && static_cast<TransitionBlock*>(block)->connected_out_block() == adjacent)
                     || (trim_mode == Timeline::kTrimOut && static_cast<TransitionBlock*>(block)->connected_in_block() == adjacent)))) {
          adjacent = nullptr;
        }

        Timeline::MovementMode flipped_mode = FlipTrimMode(trim_mode);
        TimelineViewGhostItem* adjacent_ghost;

        if (adjacent) {
          adjacent_ghost = AddGhostFromBlock(adjacent, flipped_mode);
        } else if (trim_mode == Timeline::kTrimIn || block->next()) {
          rational null_ghost_pos = (trim_mode == Timeline::kTrimIn) ? block->in() : block->out();

          adjacent_ghost = AddGhostFromNull(null_ghost_pos, null_ghost_pos, clip_item->track()->ToReference(), flipped_mode);
        } else {
          adjacent_ghost = nullptr;
        }

        // If we have an adjacent block (for any reason), this is a roll edit and the adjacent is
        // expected to fill the remaining space (no gap needs to be created)
        ghost->SetData(TimelineViewGhostItem::kTrimIsARollEdit, static_cast<bool>(adjacent));

        if (adjacent_ghost) {
          if (treat_trim_as_slide) {
            // We're sliding a transition rather than a pure trim/roll
            SetGhostToSlideMode(adjacent_ghost);
          } else if (block->type() == Block::kGap) {
            ghost->SetData(TimelineViewGhostItem::kTrimShouldBeIgnored, true);
          } else {
            adjacent_ghost->SetData(TimelineViewGhostItem::kTrimShouldBeIgnored, true);
          }
        }
      }
    }
  }
}

void PointerTool::ProcessDrag(const TimelineCoordinate &mouse_pos)
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
  if (track_movement != 0) {
    QVector<TimelineViewGhostItem*> validate_track_ghosts = parent()->GetGhostItems();
    for (int i=0;i<validate_track_ghosts.size();i++) {
      if (validate_track_ghosts.at(i)->GetTrack().type() != drag_track_type_) {
        validate_track_ghosts.removeAt(i);
        i--;
      }
    }
    track_movement = ValidateTrackMovement(track_movement, validate_track_ghosts);
  }

  // Perform movement
  foreach (TimelineViewGhostItem* ghost, parent()->GetGhostItems()) {
    switch (ghost->GetMode()) {
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
      if (ghost->GetTrack().type() == drag_track_type_) {
        ghost->SetTrackAdjustment(track_movement);
      }
      break;
    }
    }
  }

  // Regenerate tooltip and force it to update (otherwise the tooltip won't move as written in the
  // documentation, and could get in the way of the cursor)
  rational tooltip_timebase = parent()->GetTimebaseForTrackType(drag_start_.GetTrack().type());
  QToolTip::hideText();
  QToolTip::showText(QCursor::pos(),
                     Timecode::timestamp_to_timecode(Timecode::time_to_timestamp(time_movement, tooltip_timebase),
                                                     tooltip_timebase,
                                                     Core::instance()->GetTimecodeDisplay(),
                                                     true),
                     parent());
}

struct GhostBlockPair {
  TimelineViewGhostItem* ghost;
  Block* block;
};

void PointerTool::FinishDrag(TimelineViewMouseEvent *event)
{
  QList<GhostBlockPair> blocks_moving;
  QList<GhostBlockPair> blocks_sliding;
  QList<GhostBlockPair> blocks_trimming;

  // Sort ghosts depending on which ones are trimming, which are moving, and which are sliding
  foreach (TimelineViewGhostItem* ghost, parent()->GetGhostItems()) {
    if (ghost->HasBeenAdjusted()) {
      Block* b = Node::ValueToPtr<Block>(ghost->GetData(TimelineViewGhostItem::kAttachedBlock));

      if (ghost->GetData(TimelineViewGhostItem::kGhostIsSliding).toBool()) {
        blocks_sliding.append({ghost, b});
      } else if (ghost->GetMode() == Timeline::kMove) {
        blocks_moving.append({ghost, b});
      } else if (Timeline::IsATrimMode(ghost->GetMode())) {
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

  QUndoCommand* command = new QUndoCommand();

  if (!blocks_trimming.isEmpty()) {
    foreach (const GhostBlockPair& p, blocks_trimming) {
      TimelineViewGhostItem* ghost = p.ghost;

      if (!ghost->GetData(TimelineViewGhostItem::kTrimShouldBeIgnored).toBool()) {
        // Must be an ordinary trim/roll
        BlockTrimCommand* c = new BlockTrimCommand(parent()->GetTrackFromReference(ghost->GetAdjustedTrack()),
                                                   p.block,
                                                   ghost->GetAdjustedLength(),
                                                   ghost->GetMode(),
                                                   command);

        c->SetTrimIsARollEdit(ghost->GetData(TimelineViewGhostItem::kTrimIsARollEdit).toBool());
      }
    }

    if (blocks_moving.isEmpty() && blocks_sliding.isEmpty()) {
      // Trim selections (deferring to moving/sliding blocks when necessary)
      TimelineWidgetSelections new_sel = parent()->GetSelections();
      TimelineViewGhostItem* reference_ghost = blocks_trimming.first().ghost;
      if (reference_ghost->GetMode() == Timeline::kTrimIn) {
        new_sel.TrimIn(reference_ghost->GetInAdjustment());
      } else {
        new_sel.TrimOut(reference_ghost->GetOutAdjustment());
      }
      new TimelineWidget::SetSelectionsCommand(parent(), new_sel, parent()->GetSelections(), command);
    }
  }

  if (!blocks_moving.isEmpty()) {
    // See if we're duplicated because ALT is held (only moved blocks can duplicate)
    bool duplicate_clips = (event->GetModifiers() & Qt::AltModifier);
    bool inserting = (event->GetModifiers() & Qt::ControlModifier);

    // If we're not duplicating, "remove" the clips and replace them with gaps
    if (!duplicate_clips) {
      QVector<Block*> blocks_to_delete(blocks_moving.size());

      for (int i=0; i<blocks_moving.size(); i++) {
        blocks_to_delete[i] = blocks_moving.at(i).block;
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
        Node* copy;

        if (Config::Current()[QStringLiteral("SplitClipsCopyNodes")].toBool()) {
          QVector<Node*> nodes_to_clone;
          nodes_to_clone.append(block);
          nodes_to_clone.append(block->GetDependencies());
          QVector<Node*> duplicated = Node::CopyDependencyGraph(nodes_to_clone, command);
          copy = duplicated.first();
        } else {
          copy = block->copy();

          new NodeAddCommand(static_cast<NodeGraph*>(block->parent()),
                             copy,
                             command);

          new NodeCopyInputsCommand(block, copy, true, command);
        }

        // Place the copy instead of the original block
        block = static_cast<Block*>(copy);
      }

      const Track::Reference& track_ref = p.ghost->GetAdjustedTrack();
      new TrackPlaceBlockCommand(parent()->GetConnectedNode()->track_list(track_ref.type()),
                                 track_ref.index(),
                                 block,
                                 p.ghost->GetAdjustedIn(),
                                 command);
    }

    // Adjust selections
    TimelineWidgetSelections new_sel = parent()->GetSelections();
    new_sel.ShiftTime(blocks_moving.first().ghost->GetInAdjustment());
    new_sel.ShiftTracks(drag_track_type_, blocks_moving.first().ghost->GetTrackAdjustment());
    new TimelineWidget::SetSelectionsCommand(parent(), new_sel, parent()->GetSelections(), command);
  }

  if (!blocks_sliding.isEmpty()) {
    // Assume that the blocks are contiguous per track as set up in InitiateGhostsInternal()

    // All we need to do is sort them by track and order them
    QHash<Track::Reference, QList<Block*> > slide_info;
    QHash<Track::Reference, Block*> in_adjacents;
    QHash<Track::Reference, Block*> out_adjacents;
    rational movement;

    foreach (const GhostBlockPair& p, blocks_sliding) {
      const Track::Reference& track = p.ghost->GetTrack();

      switch (p.ghost->GetMode()) {
      case Timeline::kNone:
        break;
      case Timeline::kMove:
      {
        // These all should have moved uniformly, so as long as this is set, it should be fine
        movement = p.ghost->GetInAdjustment();

        QList<Block*>& blocks_on_this_track = slide_info[track];
        bool inserted = false;

        for (int i=0;i<blocks_on_this_track.size();i++) {
          if (blocks_on_this_track.at(i)->in() > p.block->in()) {
            blocks_on_this_track.insert(i, p.block);
            inserted = true;
            break;
          }
        }

        if (!inserted) {
          blocks_on_this_track.append(p.block);
        }
        break;
      }
      case Timeline::kTrimIn:
        out_adjacents.insert(track, p.block);
        break;
      case Timeline::kTrimOut:
        in_adjacents.insert(track, p.block);
        break;
      }
    }

    if (!movement.isNull()) {
      QHash<Track::Reference, QList<Block*> >::const_iterator i;
      for (i=slide_info.constBegin(); i!=slide_info.constEnd(); i++) {
        new TrackSlideCommand(parent()->GetTrackFromReference(i.key()),
                              i.value(),
                              in_adjacents.value(i.key()),
                              out_adjacents.value(i.key()),
                              movement,
                              command);
      }

      // Adjust selections
      TimelineWidgetSelections new_sel = parent()->GetSelections();
      new_sel.ShiftTime(movement);
      new TimelineWidget::SetSelectionsCommand(parent(), new_sel, parent()->GetSelections(), command);
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

Timeline::MovementMode PointerTool::IsCursorInTrimHandle(Block *block, qreal cursor_x)
{
  const double kTrimHandle = QtUtils::QFontMetricsWidth(parent()->fontMetrics(), "H");

  double block_left = parent()->TimeToScene(block->in());
  double block_right = parent()->TimeToScene(block->out());
  double block_width = block_right - block_left;

  // Block is too narrow, no trimming allowed
  if (block_width <= kTrimHandle * 2) {
    return Timeline::kNone;
  }

  if (trimming_allowed_ && cursor_x <= block_left + kTrimHandle) {
    return Timeline::kTrimIn;
  } else if (trimming_allowed_ && cursor_x >= block_right - kTrimHandle) {
    return Timeline::kTrimOut;
  } else {
    return Timeline::kNone;
  }
}

void PointerTool::InitiateDrag(Block *clicked_item,
                               Timeline::MovementMode trim_mode)
{
  InitiateDragInternal(clicked_item, trim_mode, false, false, false);
}

//#define HIDE_GAP_GHOSTS

TimelineViewGhostItem* PointerTool::AddGhostFromBlock(Block* block, Timeline::MovementMode mode, bool check_if_exists)
{
  if (check_if_exists) {
    foreach (TimelineViewGhostItem* ghost, parent()->GetGhostItems()) {
      if (Node::ValueToPtr<Block>(ghost->GetData(TimelineViewGhostItem::kAttachedBlock)) == block) {
        return ghost;
      }
    }
  }

  TimelineViewGhostItem* ghost = TimelineViewGhostItem::FromBlock(block);

#ifdef HIDE_GAP_GHOSTS
  if (block->type() == Block::kGap) {
    ghost->SetInvisible(true);
  }
#endif

  AddGhostInternal(ghost, mode);

  return ghost;
}

TimelineViewGhostItem* PointerTool::AddGhostFromNull(const rational &in, const rational &out, const Track::Reference& track, Timeline::MovementMode mode)
{
  TimelineViewGhostItem* ghost = new TimelineViewGhostItem();

  ghost->SetIn(in);
  ghost->SetOut(out);
  ghost->SetTrack(track);

#ifdef HIDE_GAP_GHOSTS
  ghost->SetInvisible(true);
#endif

  AddGhostInternal(ghost, mode);

  return ghost;
}

void PointerTool::AddGhostInternal(TimelineViewGhostItem* ghost, Timeline::MovementMode mode)
{
  ghost->SetMode(mode);

  // Prepare snap points (optimizes snapping for later)
  switch (mode) {
  case Timeline::kMove:
    snap_points_.append(ghost->GetIn());
    snap_points_.append(ghost->GetOut());
    break;
  case Timeline::kTrimIn:
    snap_points_.append(ghost->GetIn());
    break;
  case Timeline::kTrimOut:
    snap_points_.append(ghost->GetOut());
    break;
  default:
    break;
  }

  parent()->AddGhost(ghost);
}

bool PointerTool::IsClipTrimmable(Block *clip,
                                  const QVector<Block*>& items,
                                  const Timeline::MovementMode& mode)
{
  foreach (Block* compare, items) {
    if (clip->track() == compare->track()
        && clip != compare
        && ((compare->in() < clip->in() && mode == Timeline::kTrimIn)
            || (compare->out() > clip->out() && mode == Timeline::kTrimOut))) {
      return false;
    }
  }

  return true;
}

bool PointerTool::AddMovingTransitionsToClipGhost(Block* block,
                                                  Timeline::MovementMode movement,
                                                  const QVector<Block *> &selected_items)
{
  // Assume block is a clip and see if it has any transitions
  TransitionBlock* transitions[2];

  if (movement == Timeline::kMove || movement == Timeline::kTrimOut) {
    transitions[0] = block->out_transition();
  } else {
    transitions[0] = nullptr;
  }

  if (movement == Timeline::kMove || movement == Timeline::kTrimIn) {
    transitions[1] = block->in_transition();
  } else {
    transitions[1] = nullptr;
  }

  bool ret = false;

  for (int i=0;i<2;i++) {
    if (!transitions[i]) {
      continue;
    }

    bool found = false;

    foreach (Block* item, selected_items) {
      if (item == transitions[i]) {
        // Do nothing
        found = true;
        break;
      }
    }

    if (!found) {
      TimelineViewGhostItem* transition_ghost = AddGhostFromBlock(transitions[i], Timeline::kMove);

      Q_UNUSED(transition_ghost)

      ret = true;
    }
  }

  return ret;
}

rational PointerTool::ValidateInTrimming(rational movement)
{
  bool first_ghost = true;

  foreach (TimelineViewGhostItem* ghost, parent()->GetGhostItems()) {
    if (ghost->GetMode() != Timeline::kTrimIn) {
      continue;
    }

    rational earliest_in = RATIONAL_MIN;
    rational latest_in = ghost->GetOut();

    rational ghost_timebase = parent()->GetTimebaseForTrackType(ghost->GetTrack().type());

    // If the ghost must be at least one frame in size, limit the latest allowed in point
    if (!ghost->CanHaveZeroLength()) {
      latest_in -= ghost_timebase;
    }

    // Clamp adjusted value between the earliest and latest values
    rational adjusted = ghost->GetIn() + movement;
    rational clamped = clamp(adjusted, earliest_in, latest_in);

    if (clamped != adjusted) {
      movement = clamped - ghost->GetIn();
    }

    if (first_ghost) {
      movement = SnapMovementToTimebase(ghost->GetIn(), movement, ghost_timebase);
      first_ghost = false;
    }
  }

  return movement;
}

rational PointerTool::ValidateOutTrimming(rational movement)
{
  bool first_ghost = true;

  foreach (TimelineViewGhostItem* ghost, parent()->GetGhostItems()) {
    if (ghost->GetMode() != Timeline::kTrimOut) {
      continue;
    }

    // Determine earliest and latest out points
    rational earliest_out = ghost->GetIn();

    rational ghost_timebase = parent()->GetTimebaseForTrackType(ghost->GetTrack().type());

    if (!ghost->CanHaveZeroLength()) {
      earliest_out += ghost_timebase;
    }

    rational latest_out = RATIONAL_MAX;

    // Clamp adjusted value between the earliest and latest values
    rational adjusted = ghost->GetOut() + movement;
    rational clamped = clamp(adjusted, earliest_out, latest_out);

    if (clamped != adjusted) {
      movement = clamped - ghost->GetOut();
    }

    if (first_ghost) {
      movement = SnapMovementToTimebase(ghost->GetOut(), movement, ghost_timebase);
      first_ghost = false;
    }
  }

  return movement;
}

}
