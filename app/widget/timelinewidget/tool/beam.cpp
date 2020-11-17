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

#include "beam.h"
#include "widget/timelinewidget/timelinewidget.h"

OLIVE_NAMESPACE_ENTER

BeamTool::BeamTool(TimelineWidget *parent) :
  TimelineTool(parent)
{
}

void BeamTool::HoverMove(TimelineViewMouseEvent *event)
{
  parent()->SetViewBeamCursor(ValidatedCoordinate(event->GetCoordinates(true)));
}

TimelineCoordinate BeamTool::ValidatedCoordinate(TimelineCoordinate coord)
{
  if (Core::instance()->snapping()) {
    rational movement;
    parent()->SnapPoint({coord.GetFrame()}, &movement);
    if (!movement.isNull()) {
      coord.SetFrame(coord.GetFrame() + movement);
    }
  }

  return coord;
}

OLIVE_NAMESPACE_EXIT
