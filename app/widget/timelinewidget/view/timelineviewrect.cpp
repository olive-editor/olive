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

#include "timelineviewrect.h"

OLIVE_NAMESPACE_ENTER

TimelineViewRect::TimelineViewRect(QGraphicsItem* parent) :
  QGraphicsRectItem(parent),
  y_(0),
  height_(0)
{

}

void TimelineViewRect::SetYCoords(int y, int height)
{
  y_ = y;
  height_ = height;

  UpdateRect();
}

const TrackReference &TimelineViewRect::Track()
{
  return track_;
}

void TimelineViewRect::SetTrack(const TrackReference &track)
{
  track_ = track;
}

void TimelineViewRect::ScaleChangedEvent(const double &scale)
{
  TimelineScaledObject::ScaleChangedEvent(scale);

  UpdateRect();
}

void TimelineViewRect::TimebaseChangedEvent(const rational &tb)
{
  TimelineScaledObject::TimebaseChangedEvent(tb);

  UpdateRect();
}

OLIVE_NAMESPACE_EXIT
