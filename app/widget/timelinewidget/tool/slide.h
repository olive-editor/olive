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

#ifndef SLIDETIMELINETOOL_H
#define SLIDETIMELINETOOL_H

#include "pointer.h"

namespace olive {

class SlideTool : public PointerTool
{
public:
  SlideTool(TimelineWidget* parent);

protected:
  virtual void InitiateDrag(Block* clicked_item,
                            Timeline::MovementMode trim_mode) override;

};

}

#endif // SLIDETIMELINETOOL_H
