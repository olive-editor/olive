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

TimelineView::ZoomTool::ZoomTool(TimelineView* parent) :
  Tool(parent)
{
}

void TimelineView::ZoomTool::MousePress(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)
}

void TimelineView::ZoomTool::MouseMove(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)
}

void TimelineView::ZoomTool::MouseRelease(TimelineViewMouseEvent *event)
{
  double scale = parent()->scale_;

  // Get center of viewport
  QPoint viewport_center = parent()->viewport()->rect().center();
  QPointF scene_center = parent()->mapToScene(viewport_center);

  // Set X to time
  scene_center.setX(parent()->TimeToScreenCoord(event->GetCoordinates().GetFrame()));

  // Normalize zoom location for 1.0 scale
  double scaled_x = scene_center.x() / scale;

  if (event->GetModifiers() & Qt::AltModifier) {
    // Zoom out if the user clicks while holding Alt
    scale *= 0.5;
  } else {
    // Otherwise zoom in
    scale *= 2;
  }

  parent()->SetScale(scale);
  emit parent()->ScaleChanged(scale);

  // Adjust zoom location for new scale
  scaled_x *= scale;
  scene_center.setX(scaled_x);

  parent()->centerOn(scene_center);
}
