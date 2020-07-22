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

#include "node/block/gap/gap.h"
#include "widget/nodeview/nodeviewundo.h"

OLIVE_NAMESPACE_ENTER

TimelineWidget::RippleTool::RippleTool(TimelineWidget* parent) :
  PointerTool(parent)
{
  SetMovementAllowed(false);
  SetGapTrimmingAllowed(true);
}

void TimelineWidget::RippleTool::InitiateDrag(TimelineViewBlockItem *clicked_item,
                                              Timeline::MovementMode trim_mode)
{
  InitiateDragInternal(clicked_item, trim_mode, kPointer, true);

  if (parent()->ghost_items_.isEmpty()) {
    return;
  }

  // Find the earliest ripple
  rational earliest_ripple = RATIONAL_MAX;

  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    rational ghost_ripple_point;

    if (trim_mode == Timeline::kTrimIn) {
      ghost_ripple_point = ghost->In();
    } else {
      ghost_ripple_point = ghost->Out();
    }

    earliest_ripple = qMin(earliest_ripple, ghost_ripple_point);
  }

  // For each track that does NOT have a ghost, we need to make one for Gaps
  foreach (TrackOutput* track, parent()->GetConnectedNode()->GetTracks()) {
    if (track->IsLocked()) {
      continue;
    }

    // Determine if we've already created a ghost on this track
    bool ghost_on_this_track_exists = false;

    foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
      if (parent()->GetTrackFromReference(ghost->Track()) == track) {
        ghost_on_this_track_exists = true;
        break;
      }
    }

    // If there's no ghost on this track, create one
    if (!ghost_on_this_track_exists) {
      // Find the block that starts just before the ripple point, and ends either on or just after it
      Block* block_before_ripple = track->NearestBlockBefore(earliest_ripple);

      // If block is null, there will be no blocks after to ripple
      if (block_before_ripple) {
        TimelineViewGhostItem* ghost;

        TrackReference track_ref(track->track_type(), track->Index());

        if (block_before_ripple->type() == Block::kGap) {
          // If this Block is already a Gap, ghost it now
          ghost = AddGhostFromBlock(block_before_ripple, track_ref, trim_mode, true);
        } else if (block_before_ripple->next()) {
          // Assuming this block is NOT at the end of the track (i.e. next != null)

          // We're going to create a gap after it. If next is a gap, we can just use that
          if (block_before_ripple->next()->type() == Block::kGap) {
            ghost = AddGhostFromBlock(block_before_ripple->next(), track_ref, trim_mode, true);
          } else {
            // If next is NOT a gap, we'll need to create one, for which we'll use a null ghost
            ghost = AddGhostFromNull(block_before_ripple->out(), block_before_ripple->out(), track_ref, trim_mode, true);
            ghost->setData(TimelineViewGhostItem::kReferenceBlock, Node::PtrToValue(block_before_ripple));
          }
        }
      }
    }
  }
}

void TimelineWidget::RippleTool::FinishDrag(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)

  QVector< QList<TrackListRippleToolCommand::RippleInfo> > info_list(Timeline::kTrackTypeCount);

  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    TrackOutput* track = parent()->GetTrackFromReference(ghost->Track());

    TrackListRippleToolCommand::RippleInfo i = {Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock)),
                                                Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kReferenceBlock)),
                                                track,
                                                ghost->AdjustedLength(),
                                                ghost->Length()};

    info_list[track->track_type()].append(i);
  }

  QUndoCommand* command = new QUndoCommand();

  for (int i=0;i<info_list.size();i++) {
    new TrackListRippleToolCommand(parent()->GetConnectedNode()->track_list(static_cast<Timeline::TrackType>(i)),
                                   info_list.at(i),
                                   drag_movement_mode(),
                                   command);
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

OLIVE_NAMESPACE_EXIT
