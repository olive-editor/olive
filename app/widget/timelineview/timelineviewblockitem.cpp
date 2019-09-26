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

#include "timelineviewblockitem.h"

#include <QBrush>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

TimelineViewBlockItem::TimelineViewBlockItem(QGraphicsItem* parent) :
  TimelineViewRect(parent),
  block_(nullptr)
{
  setBrush(Qt::white);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
}

Block *TimelineViewBlockItem::block()
{
  return block_;
}

void TimelineViewBlockItem::SetBlock(Block *block)
{
  block_ = block;

  UpdateRect();
}

void TimelineViewBlockItem::UpdateRect()
{
  if (block_ == nullptr) {
    return;
  }

  double item_left = TimeToScreenCoord(block_->in());
  double item_width = TimeToScreenCoord(block_->length());

  // -1 on width and height so we don't overlap any adjacent clips
  setRect(0, y_, item_width - 1, height_ - 1);
  setPos(item_left, 0.0);

  // FIXME: Untranslated
  setToolTip(QString("%1\n\nIn: %2\nOut: %3\nMedia In: %4").arg(block_->Name(),
                                                                QString::number(block_->in().toDouble()),
                                                                QString::number(block_->out().toDouble()),
                                                                QString::number(block_->media_in().toDouble())));
}

void TimelineViewBlockItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget)

  if (block_ == nullptr || block_->type() == Block::kGap) {
    return;
  }

  QLinearGradient grad;
  grad.setStart(0, rect().top());
  grad.setFinalStop(0, rect().bottom());
  grad.setColorAt(0.0, QColor(160, 160, 240));
  grad.setColorAt(1.0, QColor(128, 128, 192));
  painter->fillRect(rect(), grad);
//  painter->fillRect(rect(), QColor(128, 128, 192));

  if (option->state & QStyle::State_Selected) {
    painter->fillRect(rect(), QColor(0, 0, 0, 64));
  }

  painter->setPen(Qt::white);
  painter->drawLine(rect().topLeft(), QPointF(rect().right(), rect().top()));
  painter->drawLine(rect().topLeft(), QPointF(rect().left(), rect().bottom() - 1));

  painter->drawText(rect(), static_cast<int>(Qt::AlignLeft | Qt::AlignTop), block_->block_name());

  painter->setPen(QColor(64, 64, 64));
  painter->drawLine(QPointF(rect().left(), rect().bottom() - 1), QPointF(rect().right(), rect().bottom() - 1));
  painter->drawLine(QPointF(rect().right(), rect().bottom() - 1), QPointF(rect().right(), rect().top()));
}
