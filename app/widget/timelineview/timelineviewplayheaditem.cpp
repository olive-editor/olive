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

#include "timelineviewplayheaditem.h"

#include <QDebug>
#include <QGraphicsScene>
#include <QPainter>
#include <QtMath>

TimelineViewPlayheadItem::TimelineViewPlayheadItem(QGraphicsItem *parent) :
  TimelineViewRect(parent),
  playhead_(0)
{
  // Ensure this item is always on top
  setZValue(100);
}

void TimelineViewPlayheadItem::SetPlayhead(const int64_t &playhead)
{
  playhead_ = playhead;

  UpdateRect();
}

void TimelineViewPlayheadItem::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;

  UpdateRect();
}

void TimelineViewPlayheadItem::UpdateRect()
{
  double x = TimeToScreenCoord(rational(playhead_ * timebase_.numerator(), timebase_.denominator()));
  double width = TimeToScreenCoord(timebase_);

  setRect(0, 0, width, scene()->height()-1);
  setPos(x, 0);
}

void TimelineViewPlayheadItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(option)
  Q_UNUSED(widget)

  // FIXME: Make adjustable through CSS
  painter->setPen(Qt::NoPen);
  painter->setBrush(style_.PlayheadHighlightColor());
  painter->drawRect(rect());

  painter->setPen(style_.PlayheadColor());
  painter->setBrush(Qt::NoBrush);
  painter->drawLine(QLineF(rect().topLeft(), rect().bottomLeft()));
}
