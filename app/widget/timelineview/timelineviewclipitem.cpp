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

#include "timelineviewclipitem.h"

#include <QBrush>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

TimelineViewClipItem::TimelineViewClipItem(QGraphicsItem* parent) :
  TimelineViewRect(parent),
  clip_(nullptr)
{
  setBrush(Qt::white);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
}

ClipBlock *TimelineViewClipItem::clip()
{
  return clip_;
}

void TimelineViewClipItem::SetClip(ClipBlock *clip)
{
  clip_ = clip;

  UpdateRect();
}

void TimelineViewClipItem::UpdateRect()
{
  if (clip_ == nullptr) {
    return;
  }

  double item_left = TimeToScreenCoord(clip_->in());
  double item_width = TimeToScreenCoord(clip_->length());

  setRect(0, 0, item_width - 1, 100);
  setPos(item_left, 0.0);
}

void TimelineViewClipItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget)

  /*QLinearGradient grad;
  grad.setStart(0, 0);
  grad.setFinalStop(0, rect().height());
  grad.setColorAt(0.0, QColor(192, 192, 255));
  grad.setColorAt(1.0, QColor(128, 128, 192));
  painter->fillRect(rect(), grad);*/
  painter->fillRect(rect(), QColor(128, 128, 192));

  if (option->state & QStyle::State_Selected) {
    painter->fillRect(rect(), QColor(0, 0, 0, 64));
  }

  painter->setPen(Qt::white);
  painter->drawLine(rect().topLeft(), QPointF(rect().right(), rect().top()));
  painter->drawLine(rect().topLeft(), QPointF(rect().left(), rect().bottom() - 1));

  painter->setPen(QColor(64, 64, 64));
  painter->drawLine(QPointF(rect().left(), rect().bottom() - 1), QPointF(rect().right(), rect().bottom() - 1));
  painter->drawLine(QPointF(rect().right(), rect().bottom() - 1), QPointF(rect().right(), rect().top()));
}
