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

#include "timelineviewghostitem.h"

#include <QPainter>

TimelineViewGhostItem::TimelineViewGhostItem(QGraphicsItem *parent) :
  TimelineViewRect(parent)
{
  setBrush(Qt::NoBrush);
  setPen(QPen(Qt::yellow, 2)); // FIXME: Make customizable via CSS
}

void TimelineViewGhostItem::SetIn(const rational &in)
{
  in_ = in;

  UpdateRect();
}

void TimelineViewGhostItem::SetOut(const rational &out)
{
  out_ = out;

  UpdateRect();
}

void TimelineViewGhostItem::UpdateRect()
{
  rational length = out_ - in_;

  setRect(0, 0, TimeToScreenCoord(length), 100);

  setPos(TimeToScreenCoord(in_), 0);
}
