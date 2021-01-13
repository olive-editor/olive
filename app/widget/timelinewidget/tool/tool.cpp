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

#include "node/block/transition/transition.h"

namespace olive {

TimelineTool::TimelineTool(TimelineWidget *parent) :
  dragging_(false),
  parent_(parent)
{
}

TimelineTool::~TimelineTool()
{
}

TimelineWidget *TimelineTool::parent()
{
  return parent_;
}

Timeline::MovementMode TimelineTool::FlipTrimMode(const Timeline::MovementMode &trim_mode)
{
  if (trim_mode == Timeline::kTrimIn) {
    return Timeline::kTrimOut;
  }

  if (trim_mode == Timeline::kTrimOut) {
    return Timeline::kTrimIn;
  }

  return trim_mode;
}

rational TimelineTool::SnapMovementToTimebase(const rational &start, rational movement, const rational &timebase)
{
  rational proposed_position = start + movement;
  rational snapped = Timecode::snap_time_to_timebase(proposed_position, timebase);

  if (proposed_position != snapped) {
    movement += snapped - proposed_position;
  }

  return movement;
}

rational TimelineTool::ValidateTimeMovement(rational movement)
{
  bool first_ghost = true;

  foreach (TimelineViewGhostItem* ghost, parent()->GetGhostItems()) {
    if (ghost->GetMode() != Timeline::kMove) {
      continue;
    }

    // Prevents any ghosts from going below 0:00:00 time
    if (ghost->GetIn() + movement < 0) {
      movement = -ghost->GetIn();
    } else if (first_ghost) {
      // Ensure ghost is snapped to a grid
      movement = SnapMovementToTimebase(ghost->GetIn(), movement, parent()->GetTimebaseForTrackType(ghost->GetTrack().type()));

      first_ghost = false;
    }
  }

  return movement;
}

int TimelineTool::ValidateTrackMovement(int movement, const QVector<TimelineViewGhostItem*>& ghosts)
{
  foreach (TimelineViewGhostItem* ghost, ghosts) {
    if (ghost->GetMode() != Timeline::kMove) {
      continue;
    }

    if (!ghost->GetCanMoveTracks()) {

      return 0;

    } else if (ghost->GetTrack().index() + movement < 0) {

      // Prevents any ghosts from going to a non-existent negative track
      movement = -ghost->GetTrack().index();

    }
  }

  return movement;
}

void TimelineTool::GetGhostData(rational *earliest_point, rational *latest_point)
{
  rational ep = RATIONAL_MAX;
  rational lp = RATIONAL_MIN;

  foreach (TimelineViewGhostItem* ghost, parent()->GetGhostItems()) {
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

void TimelineTool::InsertGapsAtGhostDestination(QUndoCommand *command)
{
  rational earliest_point, latest_point;

  GetGhostData(&earliest_point, &latest_point);

  parent()->InsertGapsAt(earliest_point, latest_point - earliest_point, command);
}

}
