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

#include "timelineplayhead.h"

#include <QPainter>

const QColor &TimelinePlayhead::GetPlayheadColor() const
{
  return playhead_color_;
}

const QColor &TimelinePlayhead::GetPlayheadHighlightColor() const
{
  return playhead_highlight_color_;
}

void TimelinePlayhead::SetPlayheadColor(QColor c)
{
  playhead_color_ = c;
}

void TimelinePlayhead::SetPlayheadHighlightColor(QColor c)
{
  playhead_highlight_color_ = c;
}

void TimelinePlayhead::Draw(QPainter* painter, const QRectF& playhead_rect) const
{
  painter->setPen(Qt::NoPen);
  painter->setBrush(GetPlayheadHighlightColor());
  painter->drawRect(playhead_rect);

  painter->setPen(GetPlayheadColor());
  painter->setBrush(Qt::NoBrush);
  painter->drawLine(QLineF(playhead_rect.topLeft(), playhead_rect.bottomLeft()));
}
