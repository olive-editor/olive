/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef TRACKSELECTTOOL_H
#define TRACKSELECTTOOL_H

#include "pointer.h"

namespace olive {

class TrackSelectTool : public PointerTool
{
public:
  TrackSelectTool(TimelineWidget *parent);

  virtual void MousePress(TimelineViewMouseEvent *event) override;

private:
  void SelectBlocksOnTrack(Track *track, TimelineViewMouseEvent *event, QVector<Block *> *blocks, bool forward);

};

}

#endif // TRACKSELECTTOOL_H
