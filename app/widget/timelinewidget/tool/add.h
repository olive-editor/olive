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

#ifndef ADDTIMELINETOOL_H
#define ADDTIMELINETOOL_H

#include "beam.h"

OLIVE_NAMESPACE_ENTER

class AddTool : public BeamTool
{
public:
  AddTool(TimelineWidget* parent);

  virtual void MousePress(TimelineViewMouseEvent *event) override;
  virtual void MouseMove(TimelineViewMouseEvent *event) override;
  virtual void MouseRelease(TimelineViewMouseEvent *event) override;

protected:
  void MouseMoveInternal(const rational& cursor_frame, bool outwards);

  TimelineViewGhostItem* ghost_;

  rational drag_start_point_;
};

OLIVE_NAMESPACE_EXIT

#endif // ADDTIMELINETOOL_H
