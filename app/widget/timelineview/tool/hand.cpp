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

#include <QScrollBar>

TimelineView::HandTool::HandTool(TimelineView* parent) :
  Tool(parent)
{

}

void TimelineView::HandTool::MousePress(QMouseEvent *event)
{
  screen_drag_start_ = event->pos();
  scrollbar_start_.setX(parent()->horizontalScrollBar()->value());
  scrollbar_start_.setY(parent()->verticalScrollBar()->value());
  dragging_ = true;
}

void TimelineView::HandTool::MouseMove(QMouseEvent *event)
{
  if (dragging_) {
    QPoint drag_diff = event->pos() - screen_drag_start_;

    parent()->horizontalScrollBar()->setValue(scrollbar_start_.x() - drag_diff.x());
    parent()->verticalScrollBar()->setValue(scrollbar_start_.y() - drag_diff.y());
  }
}

void TimelineView::HandTool::MouseRelease(QMouseEvent *event)
{
  Q_UNUSED(event)

  dragging_ = false;
}
