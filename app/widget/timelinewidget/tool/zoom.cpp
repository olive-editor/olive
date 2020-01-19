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

  if (dragging_) {
    parent()->MoveRubberBandSelect(false, false);
  } else {
    parent()->StartRubberBandSelect(false, false);

    dragging_ = true;
  }
}

void TimelineWidget::ZoomTool::MouseRelease(TimelineViewMouseEvent *event)
{
  if (dragging_) {
    // Zoom into the rubberband selection
    QRect screen_coords = parent()->rubberband_.geometry();

    parent()->EndRubberBandSelect(false, false);

    TimelineView* reference_view = parent()->views_.first()->view();
    QPointF scene_topleft = reference_view->mapToScene(reference_view->mapFrom(parent(), screen_coords.topLeft()));
    QPointF scene_bottomright = reference_view->mapToScene(reference_view->mapFrom(parent(), screen_coords.bottomRight()));

    double scene_left = scene_topleft.x();
    double scene_right = scene_bottomright.x();

    // Normalize scale to 1.0 scale
    double scene_width = (scene_right - scene_left) / parent()->scale_;

    double new_scale = qMin(TimelineViewBase::kMaximumScale, static_cast<double>(reference_view->viewport()->width()) / scene_width);
    parent()->deferred_scroll_value_ = qMax(0, qRound(scene_left / parent()->scale_ * new_scale));

    parent()->SetScale(new_scale, false);

    dragging_ = false;
  } else {
    // Simple zoom in/out at the cursor position
    double scale = parent()->scale_;

    if (event->GetModifiers() & Qt::AltModifier) {
      // Zoom out if the user clicks while holding Alt
      scale *= 0.5;
    } else {
      // Otherwise zoom in
      scale *= 2.0;
    }

    parent()->SetScale(scale, false);

    // Adjust scroll location for new scale
    double frame_x = event->GetFrame().toDouble() * scale;

    parent()->deferred_scroll_value_ = qMax(0, qRound(frame_x - parent()->views_.first()->view()->viewport()->width()/2));
  }

  // (using a hacky singleShot so the scroll occurs after the scene and its scrollbars have updated)
  QTimer::singleShot(0, parent(), &TimelineWidget::DeferredScrollAction);
}
