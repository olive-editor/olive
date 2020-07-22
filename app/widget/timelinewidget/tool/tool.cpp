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

#include "node/block/transition/transition.h"
#include "widget/nodeview/nodeviewundo.h"

OLIVE_NAMESPACE_ENTER

TimelineWidget::Tool::Tool(TimelineWidget *parent) :
  dragging_(false),
  parent_(parent)
{
}

TimelineWidget::Tool::~Tool()
{
}

TimelineWidget *TimelineWidget::Tool::parent()
{
  return parent_;
}

Timeline::MovementMode TimelineWidget::Tool::FlipTrimMode(const Timeline::MovementMode &trim_mode)
{
  if (trim_mode == Timeline::kTrimIn) {
    return Timeline::kTrimOut;
  }

  if (trim_mode == Timeline::kTrimOut) {
    return Timeline::kTrimIn;
  }

  return trim_mode;
}

TimelineViewBlockItem *TimelineWidget::Tool::GetItemAtScenePos(const TimelineCoordinate& coord)
{
  QMapIterator<Block*, TimelineViewBlockItem*> iterator(parent()->block_items_);

  while (iterator.hasNext()) {
    iterator.next();

    Block* b = iterator.key();
    TimelineViewBlockItem* item = iterator.value();

    if (b->in() <= coord.GetFrame()
        && b->out() > coord.GetFrame()
        && item->Track() == coord.GetTrack()) {
      return item;
    }
  }

  return nullptr;
}

rational TimelineWidget::Tool::ValidateTimeMovement(rational movement)
{
  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    if (ghost->mode() != Timeline::kMove) {
      continue;
    }

    Block* block = Node::ValueToPtr<Block>(ghost->data(TimelineViewGhostItem::kAttachedBlock));

    if (block && block->type() == Block::kTransition) {
      TransitionBlock* transition = static_cast<TransitionBlock*>(block);

      // Dual transitions are only allowed to move so that neither of their offsets are < 0
      if (transition->connected_in_block() && transition->connected_out_block()) {
        if (movement > transition->out_offset()) {
          movement = transition->out_offset();
        }

        if (movement < -transition->in_offset()) {
          movement = -transition->in_offset();
        }
      }
    }

    // Prevents any ghosts from going below 0:00:00 time
    if (ghost->In() + movement < 0) {
      movement = -ghost->In();
    }
  }

  return movement;
}

int TimelineWidget::Tool::ValidateTrackMovement(int movement, const QVector<TimelineViewGhostItem*>& ghosts)
{
  foreach (TimelineViewGhostItem* ghost, ghosts) {
    if (ghost->mode() != Timeline::kMove) {
      continue;
    }

    if (!ghost->CanMoveTracks()) {

      return 0;

    } else if (ghost->Track().index() + movement < 0) {

      // Prevents any ghosts from going to a non-existent negative track
      movement = -ghost->Track().index();

    }
  }

  return movement;
}

void TimelineWidget::Tool::GetGhostData(rational *earliest_point, rational *latest_point)
{
  rational ep = RATIONAL_MAX;
  rational lp = RATIONAL_MIN;

  foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
    ep = qMin(ep, ghost->GetAdjustedIn());
    lp = qMax(lp, ghost->GetAdjustedOut());
  }

  if (earliest_point) {
    *earliest_point = ep;
  }

  if (latest_point) {
    *latest_point = lp;
  }
}

void TimelineWidget::Tool::InsertGapsAtGhostDestination(QUndoCommand *command)
{
  rational earliest_point, latest_point;

  GetGhostData(&earliest_point, &latest_point);

  parent()->InsertGapsAt(earliest_point, latest_point - earliest_point, command);
}

OLIVE_NAMESPACE_EXIT
