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

TimelineWidget::SlideTool::SlideTool(TimelineWidget* parent) :
  PointerTool(parent)
{
  SetTrimmingAllowed(false);
  SetTrackMovementAllowed(false);
  SetTrimOverwriteAllowed(true);
}

struct TrackBlockListPair {
  TrackReference track;
  QList<Block*> blocks;
};

void TimelineWidget::SlideTool::InitiateDrag(TimelineViewBlockItem *clicked_item,
                                             Timeline::MovementMode trim_mode,
                                             bool allow_gap_trimming)
{
  Q_UNUSED(allow_gap_trimming)

  PointerTool::InitiateDrag(clicked_item, trim_mode, true);

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
          AddGhostFromBlock(b, track, Timeline::kMove);
        }
      }
    }

    // Add surrounding blocks that will be trimming instead of moving
    if (earliest_block->previous()) {
      AddGhostFromBlock(earliest_block->previous(), track, Timeline::kTrimOut);
    }

    if (latest_block->next()) {
      AddGhostFromBlock(latest_block->next(), track, Timeline::kTrimIn);
    }
  }
}

void TimelineWidget::SlideTool::FinishDrag(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)

  QVector<TrackSlideCommand::BlockSlideInfo> info;

  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    if (!ghost->HasBeenAdjusted()) {
      continue;
    }

    Block* b = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    info.append({parent()->GetTrackFromReference(ghost->Track()),
                 b,
                 ghost->mode(),
                 ghost->mode() == Timeline::kMove ? ghost->GetAdjustedIn() : ghost->AdjustedLength(),
                 ghost->mode() == Timeline::kMove ? ghost->In() : ghost->Length()});
  }

  if (!info.isEmpty()) {
    Core::instance()->undo_stack()->push(new TrackSlideCommand(info));
  }
}

OLIVE_NAMESPACE_EXIT
