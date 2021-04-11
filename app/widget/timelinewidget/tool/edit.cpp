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

#include "edit.h"
#include "widget/timelinewidget/timelinewidget.h"

namespace olive {

EditTool::EditTool(TimelineWidget* parent) :
  BeamTool(parent)
{
}

void EditTool::MousePress(TimelineViewMouseEvent *event)
{
  if (!(event->GetModifiers() & Qt::ShiftModifier)) {
    parent()->DeselectAll();
  }
}

void EditTool::MouseMove(TimelineViewMouseEvent *event)
{
  if (dragging_) {
    rational end_frame = event->GetFrame(true);

    if (Core::instance()->snapping()) {
      rational movement;
      parent()->SnapPoint({end_frame}, &movement);
      if (!movement.isNull()) {
        end_frame += movement;
      }
    }

    parent()->SetSelections(start_selections_);
    parent()->AddSelection(TimeRange(start_coord_.GetFrame(), end_frame),
                           start_coord_.GetTrack());
  } else {
    start_selections_ = parent()->GetSelections();

    dragging_ = true;

    start_coord_ = event->GetCoordinates(true);

    // Snap if we're snapping
    if (Core::instance()->snapping()) {
      rational movement;
      parent()->SnapPoint({start_coord_.GetFrame()}, &movement);
      if (!movement.isNull()) {
        start_coord_.SetFrame(start_coord_.GetFrame() + movement);
      }
    }

    dragging_ = true;
  }
}

void EditTool::MouseRelease(TimelineViewMouseEvent *event)
{
  dragging_ = false;
}

void EditTool::MouseDoubleClick(TimelineViewMouseEvent *event)
{
  Block* item = parent()->GetItemAtScenePos(event->GetCoordinates());

  if (item && !item->track()->IsLocked()) {
    parent()->AddSelection(item);
  }
}

}
