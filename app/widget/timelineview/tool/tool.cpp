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
