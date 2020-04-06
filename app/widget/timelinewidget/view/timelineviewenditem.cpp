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

#include "timelineviewenditem.h"

OLIVE_NAMESPACE_ENTER

TimelineViewEndItem::TimelineViewEndItem(QGraphicsItem *parent) :
  TimelineViewRect(parent),
  end_padding_(0)
{
}

void TimelineViewEndItem::SetEndTime(const rational &time)
{
  end_time_ = time;

  UpdateRect();
}

void TimelineViewEndItem::SetEndPadding(int padding)
{
  end_padding_ = padding;

  UpdateRect();
}

void TimelineViewEndItem::UpdateRect()
{
  // Doesn't need to be more than one pixel
  setRect(0, 0, 1, 1);

  setPos(TimeToScene(end_time_) + end_padding_, 0);
}

void TimelineViewEndItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  // Item is invisible, this is a no-op
  Q_UNUSED(painter)
  Q_UNUSED(option)
  Q_UNUSED(widget)
}

OLIVE_NAMESPACE_EXIT
