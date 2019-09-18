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

void TimelineView::ZoomTool::MousePress(QMouseEvent *event)
{
  Q_UNUSED(event)
}

void TimelineView::ZoomTool::MouseMove(QMouseEvent *event)
{
  Q_UNUSED(event)
}

void TimelineView::ZoomTool::MouseRelease(QMouseEvent *event)
{
  if (event->modifiers() & Qt::AltModifier) {
    // Zoom out if the user clicks while holding Alt
    emit parent()->UserSetScale(parent()->scale_ * 0.5);
  } else {
    // Otherwise zoom in
    emit parent()->UserSetScale(parent()->scale_ * 2);
  }
}
