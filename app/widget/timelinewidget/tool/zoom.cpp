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

TimelineWidget::ZoomTool::ZoomTool(TimelineWidget *parent) :
  Tool(parent)
{
}

void TimelineWidget::ZoomTool::MousePress(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)
}

void TimelineWidget::ZoomTool::MouseMove(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)
}

void TimelineWidget::ZoomTool::MouseRelease(TimelineViewMouseEvent *event)
{
  double scale = parent()->scale_;

  // Normalize zoom location for 1.0 scale
  double frame_x = parent()->TimeToScene(event->GetCoordinates().GetFrame());

  if (event->GetModifiers() & Qt::AltModifier) {
    // Zoom out if the user clicks while holding Alt
    scale *= 0.5;
  } else {
    // Otherwise zoom in
    scale *= 2;
  }

  parent()->SetScale(scale);

  // Adjust zoom location for new scale
  frame_x *= scale;

  parent()->CenterOn(frame_x);
}
