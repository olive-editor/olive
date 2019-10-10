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

#include <float.h>

#include "common/range.h"

TimelineView::Tool::Tool(TimelineView *parent) :
  dragging_(false),
  parent_(parent),
  drag_mode_(NoDrag),
  enable_default_behavior_(false)
{
}

TimelineView::Tool::~Tool()
{
}

TimelineView *TimelineView::Tool::parent()
{
  return parent_;
}

olive::timeline::MovementMode TimelineView::Tool::FlipTrimMode(const olive::timeline::MovementMode &trim_mode)
{
  if (trim_mode == olive::timeline::kTrimIn) {
    return olive::timeline::kTrimOut;
  }

  if (trim_mode == olive::timeline::kTrimOut) {
    return olive::timeline::kTrimIn;
  }

  return trim_mode;
}

const QGraphicsView::DragMode &TimelineView::Tool::drag_mode()
{
  return drag_mode_;
}

bool TimelineView::Tool::enable_default_behavior()
{
  return enable_default_behavior_;
}

void TimelineView::Tool::set_drag_mode(const QGraphicsView::DragMode &mode)
{
  drag_mode_ = mode;
}

void TimelineView::Tool::set_enable_default_behavior(bool enable)
{
  enable_default_behavior_ = enable;
}

TimelineViewBlockItem *TimelineView::Tool::GetItemAtScenePos(const TimelineCoordinate& coord)
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

rational TimelineView::Tool::ValidateFrameMovement(rational movement, const QVector<TimelineViewGhostItem *> ghosts)
{
  foreach (TimelineViewGhostItem* ghost, ghosts) {
    if (ghost->mode() != olive::timeline::kMove) {
      continue;
    }

    // Prevents any ghosts from going below 0:00:00 time
    if (ghost->In() + movement < 0) {
      movement = -ghost->In();
    }
  }

  return movement;
}

int TimelineView::Tool::ValidateTrackMovement(int movement, const QVector<TimelineViewGhostItem *> ghosts)
{
  foreach (TimelineViewGhostItem* ghost, ghosts) {
    // Prevents any ghosts from going to a non-existent negative track
    if (ghost->Track() + movement < 0) {
      if (ghost->mode() != olive::timeline::kMove) {
        continue;
      }

      movement = -ghost->Track();
    }
  }

  return movement;
}

bool TimelineView::Tool::SnapPoint(QList<rational> start_times, rational* movement, int snap_points)
{
  QList<QGraphicsItem*> items = parent()->scene_.items();

  double diff = DBL_MAX;

  QList<double> proposed_pts;

  foreach (rational s, start_times) {
    proposed_pts.append((s + *movement).toDouble() * parent()->scale_);
  }

  if (snap_points & kSnapToPlayhead) {


    rational playhead_abs_time = rational(parent()->playhead_ * parent()->timebase_.numerator(),
                                          parent()->timebase_.denominator());

    qreal playhead_pos = playhead_abs_time.toDouble() * parent()->scale_;

    AttemptSnap(proposed_pts, playhead_pos, start_times, playhead_abs_time, movement, &diff);
  }

  if (snap_points & kSnapToClips) {
    foreach (QGraphicsItem* it, items) {
      TimelineViewBlockItem* timeline_rect = dynamic_cast<TimelineViewBlockItem*>(it);

      if (timeline_rect != nullptr) {
        qreal rect_left = timeline_rect->x();
        qreal rect_right = rect_left + timeline_rect->rect().width();

        // Attempt snapping to clip in point
        AttemptSnap(proposed_pts, rect_left, start_times, timeline_rect->block()->in(), movement, &diff);

        // Attempt snapping to clip out point
        AttemptSnap(proposed_pts, rect_right, start_times, timeline_rect->block()->out(), movement, &diff);
      }
    }
  }


  return (diff < DBL_MAX);
}
