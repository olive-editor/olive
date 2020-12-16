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
#include "zoom.h"

namespace olive {

ZoomTool::ZoomTool(TimelineWidget *parent) :
  TimelineTool(parent)
{
}

void ZoomTool::MousePress(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)

  drag_global_start_ = QCursor::pos();
}

void ZoomTool::MouseMove(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)

  if (!dragging_) {
    parent()->StartRubberBandSelect(drag_global_start_);

    dragging_ = true;
  }

  parent()->MoveRubberBandSelect(false, false);
}

void ZoomTool::MouseRelease(TimelineViewMouseEvent *event)
{
  int scroll_value;

  if (dragging_) {
    // Zoom into the rubberband selection
    QRect screen_coords = parent()->GetRubberBandGeometry();

    parent()->EndRubberBandSelect();

    TimelineView* reference_view = parent()->GetFirstTimelineView();
    QPointF scene_topleft = reference_view->mapToScene(reference_view->mapFrom(parent(), screen_coords.topLeft()));
    QPointF scene_bottomright = reference_view->mapToScene(reference_view->mapFrom(parent(), screen_coords.bottomRight()));

    double scene_left = scene_topleft.x();
    double scene_right = scene_bottomright.x();

    // Normalize scale to 1.0 scale
    double scene_width = (scene_right - scene_left) / parent()->GetScale();

    double new_scale = qMin(TimeBasedView::kMaximumScale, static_cast<double>(reference_view->viewport()->width()) / scene_width);

    parent()->SetScale(new_scale);

    scroll_value = qMax(0, qRound(scene_left / parent()->GetScale() * new_scale));

    dragging_ = false;
  } else {
    // Simple zoom in/out at the cursor position
    double scale = parent()->GetScale();

    if (event->GetModifiers() & Qt::AltModifier) {
      // Zoom out if the user clicks while holding Alt
      scale *= 0.5;
    } else {
      // Otherwise zoom in
      scale *= 2.0;
    }

    parent()->SetScale(scale);

    // Adjust scroll location for new scale
    double frame_x = event->GetFrame().toDouble() * scale;

    scroll_value = qMax(0, qRound(frame_x - parent()->GetFirstTimelineView()->viewport()->width()/2));
  }

  parent()->QueueScroll(scroll_value);
}

}
