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

#ifndef TRANSITIONTIMELINETOOL_H
#define TRANSITIONTIMELINETOOL_H

#include "add.h"

namespace olive {

class TransitionTool : public AddTool
{
public:
  TransitionTool(TimelineWidget* parent);

  virtual void MousePress(TimelineViewMouseEvent *event) override;
  virtual void MouseMove(TimelineViewMouseEvent *event) override;
  virtual void MouseRelease(TimelineViewMouseEvent *event) override;
private:
  bool dual_transition_;
};

}

#endif // TRANSITIONTIMELINETOOL_H
