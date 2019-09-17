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
  parent_(parent)
{
}

TimelineView::Tool::~Tool(){}

void TimelineView::Tool::MousePress(QMouseEvent *){}

void TimelineView::Tool::MouseMove(QMouseEvent *){}

void TimelineView::Tool::MouseRelease(QMouseEvent *){}

void TimelineView::Tool::DragEnter(QDragEnterEvent *){}

void TimelineView::Tool::DragMove(QDragMoveEvent *){}

void TimelineView::Tool::DragLeave(QDragLeaveEvent *){}

void TimelineView::Tool::DragDrop(QDropEvent *){}

TimelineView *TimelineView::Tool::parent()
{
  return parent_;
}

QPointF TimelineView::Tool::GetScenePos(const QPoint &screen_pos)
{
  return parent()->mapToScene(screen_pos);
}

QGraphicsItem *TimelineView::Tool::GetItemAtScenePos(const QPointF &scene_pos)
{
  return parent()->scene_.itemAt(scene_pos, parent()->transform());
}

rational TimelineView::Tool::ValidateFrameMovement(rational movement, const QVector<TimelineViewGhostItem *> ghosts)
{
  foreach (TimelineViewGhostItem* ghost, ghosts) {
    if (ghost->mode() != TimelineViewGhostItem::kMove) {
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
      if (ghost->mode() != TimelineViewGhostItem::kMove) {
        continue;
      }

      movement = -ghost->Track();
    }
  }

  return movement;
}

rational TimelineView::Tool::ValidateInTrimming(rational movement, const QVector<TimelineViewGhostItem *> ghosts)
{
  foreach (TimelineViewGhostItem* ghost, ghosts) {
    if (ghost->mode() != TimelineViewGhostItem::kTrimIn) {
      continue;
    }

    Block* block = Node::ValueToPtr<Block>(ghost->data(0));

    // Prevents any media_in points from becoming negative
    if (block->media_in() + movement < 0) {
      movement = -block->media_in();
    }

    // Prevents any clip length's becoming infinitely small (or negative length)
    if (ghost->In() + movement >= ghost->Out()) {
      // Since the timebase is considered more or less the "minimum unit", we adjust the movement to make the proposed
      // length precisely one timebase unit in size
      movement = ghost->Out() - parent()->timebase_ - ghost->In();
    }
  }

  return movement;
}

rational TimelineView::Tool::ValidateOutTrimming(rational movement, const QVector<TimelineViewGhostItem *> ghosts)
{
  foreach (TimelineViewGhostItem* ghost, ghosts) {
    if (ghost->mode() != TimelineViewGhostItem::kTrimOut) {
      continue;
    }

    // Prevents any clip length's becoming infinitely small (or negative length)
    if (ghost->Out() + movement <= ghost->In()) {
      // Since the timebase is considered more or less the "minimum unit", we adjust the movement to make the proposed
      // length precisely one timebase unit in size
      movement = ghost->In() + parent()->timebase_ - ghost->Out();
    }
  }

  return movement;
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

bool TimelineView::Tool::SnapPoint(QList<rational> start_times, rational* movement, int snap_points)
{
  QList<QGraphicsItem*> items = parent()->scene_.items();

  double diff = DBL_MAX;

  QList<double> proposed_pts;

  foreach (rational s, start_times) {
    proposed_pts.append((s + *movement).toDouble() * parent()->scale_);
  }

  if (snap_points & kSnapToPlayhead) {
    qreal playhead_pos = parent()->playhead_line_->x();

    rational playhead_abs_time = rational(parent()->playhead_ * parent()->timebase_.numerator(),
                                          parent()->timebase_.denominator());

    AttemptSnap(proposed_pts, playhead_pos, start_times, playhead_abs_time, movement, &diff);
  }

  if (snap_points & kSnapToClips) {
    foreach (QGraphicsItem* it, items) {
      TimelineViewClipItem* timeline_rect = dynamic_cast<TimelineViewClipItem*>(it);

      if (timeline_rect != nullptr) {
        qreal rect_left = timeline_rect->x();
        qreal rect_right = rect_left + timeline_rect->rect().width();

        // Attempt snapping to clip in point
        AttemptSnap(proposed_pts, rect_left, start_times, timeline_rect->clip()->in(), movement, &diff);

        // Attempt snapping to clip out point
        AttemptSnap(proposed_pts, rect_right, start_times, timeline_rect->clip()->out(), movement, &diff);
      }
    }
  }


  return (diff < DBL_MAX);
}
