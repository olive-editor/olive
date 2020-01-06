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

#include <float.h>

#include "common/range.h"
#include "node/block/transition/transition.h"

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

void AttemptSnap(const QList<double>& proposed_pts,
                 double compare_point,
                 const QList<rational>& start_times,
                 rational compare_time,
                 rational* movement,
                 double* diff) {
  const qreal kSnapRange = 10; // FIXME: Hardcoded number

  for (int i=0;i<proposed_pts.size();i++) {
    // Attempt snapping to clip out point
    if (InRange(proposed_pts.at(i), compare_point, kSnapRange)) {
      double this_diff = qAbs(compare_point - proposed_pts.at(i));

      if (this_diff < *diff
          && start_times.at(i) + *movement >= 0) {
        *movement = compare_time - start_times.at(i);
        *diff = this_diff;
      }
    }
  }
}

rational TimelineWidget::Tool::ValidateFrameMovement(rational movement, const QVector<TimelineViewGhostItem *> ghosts)
{
  foreach (TimelineViewGhostItem* ghost, ghosts) {
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

int TimelineWidget::Tool::ValidateTrackMovement(int movement, const QVector<TimelineViewGhostItem *> ghosts)
{
  foreach (TimelineViewGhostItem* ghost, ghosts) {
    if (ghost->mode() != Timeline::kMove) {
      continue;
    }

    if (!ghost->CanMoveTracks()) {

      movement = 0;

    } else if (ghost->Track().index() + movement < 0) {

      // Prevents any ghosts from going to a non-existent negative track
      movement = -ghost->Track().index();

    }
  }

  return movement;
}

bool TimelineWidget::Tool::SnapPoint(QList<rational> start_times, rational* movement, int snap_points)
{
  double diff = DBL_MAX;

  QList<double> proposed_pts;

  foreach (rational s, start_times) {
    proposed_pts.append((s + *movement).toDouble() * parent()->scale_);
  }

  if (snap_points & kSnapToPlayhead) {


    rational playhead_abs_time = rational(parent()->playhead_ * parent()->timebase().numerator(),
                                          parent()->timebase().denominator());

    qreal playhead_pos = playhead_abs_time.toDouble() * parent()->scale_;

    AttemptSnap(proposed_pts, playhead_pos, start_times, playhead_abs_time, movement, &diff);
  }

  if (snap_points & kSnapToClips) {
    QMapIterator<Block*, TimelineViewBlockItem*> iterator(parent()->block_items_);

    while (iterator.hasNext()) {
      iterator.next();

      TimelineViewBlockItem* item = iterator.value();

      if (item != nullptr) {
        qreal rect_left = item->x();
        qreal rect_right = rect_left + item->rect().width();

        // Attempt snapping to clip in point
        AttemptSnap(proposed_pts, rect_left, start_times, item->block()->in(), movement, &diff);

        // Attempt snapping to clip out point
        AttemptSnap(proposed_pts, rect_right, start_times, item->block()->out(), movement, &diff);
      }
    }
  }


  return (diff < DBL_MAX);
}
