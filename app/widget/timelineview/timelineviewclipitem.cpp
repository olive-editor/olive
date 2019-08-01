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
  QGraphicsRectItem(parent),
  clip_(nullptr),
  ghost_(nullptr)
{
  setBrush(Qt::white);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
}

void TimelineViewClipItem::SetClip(ClipBlock *clip)
{
  clip_ = clip;
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

void TimelineViewClipItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsRectItem::mousePressEvent(event);

  ghost_ = new TimelineViewGhostItem();
  ghost_->setRect(rect());
  scene()->addItem(ghost_);
}

void TimelineViewClipItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsRectItem::mouseMoveEvent(event);

  if (ghost_ != nullptr) {
    ghost_->setPos(event->scenePos());
  }
}

void TimelineViewClipItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsRectItem::mouseReleaseEvent(event);

  if (ghost_ != nullptr) {
    scene()->removeItem(ghost_);
    delete ghost_;
    ghost_ = nullptr;
  }
}
