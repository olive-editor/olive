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

#ifndef EDITTIMELINETOOL_H
#define EDITTIMELINETOOL_H

#include "beam.h"
#include "tool.h"
#include "widget/timelinewidget/timelinewidgetselections.h"

OLIVE_NAMESPACE_ENTER

class EditTool : public BeamTool
{
public:
  EditTool(TimelineWidget* parent);

  virtual void MousePress(TimelineViewMouseEvent *event) override;
  virtual void MouseMove(TimelineViewMouseEvent *event) override;
  virtual void MouseRelease(TimelineViewMouseEvent *event) override;
  virtual void MouseDoubleClick(TimelineViewMouseEvent *event) override;

private:
  TimelineWidgetSelections start_selections_;

  TimelineCoordinate start_coord_;

};

OLIVE_NAMESPACE_EXIT

#endif // EDITTIMELINETOOL_H
