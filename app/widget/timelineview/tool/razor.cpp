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

TimelineView::RazorTool::RazorTool(TimelineView* parent) :
  Tool(parent)
{
}

void TimelineView::RazorTool::MousePress(QMouseEvent *event)
{
  MouseMove(event);
}

void TimelineView::RazorTool::MouseMove(QMouseEvent *event)
{
  if (!dragging_) {
    drag_start_ = GetScenePos(event->pos());
    dragging_ = true;
  }

  QPointF current_scene_pos = GetScenePos(event->pos());

  // Always split at the same time
  rational split_time = parent()->SceneToTime(drag_start_.x());

  // Split at the current cursor track
  int split_track = parent()->SceneToTrack(current_scene_pos.y());

  parent()->timeline_node_->SplitAtTime(split_time, split_track);
}

void TimelineView::RazorTool::MouseRelease(QMouseEvent *event)
{
  Q_UNUSED(event)

  dragging_ = false;
}
