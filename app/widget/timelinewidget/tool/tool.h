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

#ifndef TIMELINETOOL_H
#define TIMELINETOOL_H

#include <QDragLeaveEvent>

#include "common/rational.h"
#include "widget/timelinewidget/view/timelineviewghostitem.h"
#include "widget/timelinewidget/view/timelineviewmouseevent.h"

OLIVE_NAMESPACE_ENTER

class TimelineWidget;

class TimelineTool
{
public:
  TimelineTool(TimelineWidget* parent);
  virtual ~TimelineTool();

  virtual void MousePress(TimelineViewMouseEvent *){}
  virtual void MouseMove(TimelineViewMouseEvent *){}
  virtual void MouseRelease(TimelineViewMouseEvent *){}
  virtual void MouseDoubleClick(TimelineViewMouseEvent *){}

  virtual void HoverMove(TimelineViewMouseEvent *){}

  virtual void DragEnter(TimelineViewMouseEvent *){}
  virtual void DragMove(TimelineViewMouseEvent *){}
  virtual void DragLeave(QDragLeaveEvent *){}
  virtual void DragDrop(TimelineViewMouseEvent *){}

  TimelineWidget* parent();

  static Timeline::MovementMode FlipTrimMode(const Timeline::MovementMode& trim_mode);

protected:
  /**
   * @brief Validates Ghosts that are moving horizontally (time-based)
   *
   * Validation is the process of ensuring that whatever movements the user is making are "valid" and "legal". This
   * function's validation ensures that no Ghost's in point ends up in a negative timecode.
   */
  rational ValidateTimeMovement(rational movement);

  /**
   * @brief Validates Ghosts that are moving vertically (track-based)
   *
   * This function's validation ensures that no Ghost's track ends up in a negative (non-existent) track.
   */
  int ValidateTrackMovement(int movement, const QVector<TimelineViewGhostItem *> &ghosts);

  void GetGhostData(rational *earliest_point, rational *latest_point);

  void InsertGapsAtGhostDestination(QUndoCommand* command);

  QList<rational> snap_points_;

  bool dragging_;

  TimelineCoordinate drag_start_;

private:
  TimelineWidget* parent_;

};

OLIVE_NAMESPACE_EXIT

#endif // TIMELINETOOL_H
