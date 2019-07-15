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

#include "nodeviewitem.h"

#include <QApplication>
#include <QBrush>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>
#include <QStyleOptionGraphicsItem>

const int kNodeViewItemBorderWidth = 2;
const int kNodeViewItemWidth = 250;
const int kNodeViewItemPadding = 5;

NodeViewItem::NodeViewItem(QGraphicsItem *parent) :
  QGraphicsRectItem(parent),
  node_(nullptr)
{
  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemIsSelectable);

  QFont f;
  QFontMetrics fm(f);

  setRect(0, 0, kNodeViewItemWidth, fm.height() + kNodeViewItemPadding * 2);
}

void NodeViewItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget)

  // Set up border, which will change color if selected
  QPen pen;

  pen.setWidth(kNodeViewItemBorderWidth);

  if (option->state & QStyle::State_Selected) {
    pen.setColor(qApp->palette().highlight().color());
  } else {
    // FIXME: Not configurable?
    pen.setColor(Qt::black);
  }

  // Draw rect
  painter->setPen(pen);
  painter->setBrush(qApp->palette().window());
  painter->drawRect(rect());


  // Draw text
  if (node_ != nullptr) {
    painter->setPen(qApp->palette().text().color());

    QRectF text_rect = rect();
    text_rect.adjust(kNodeViewItemPadding, kNodeViewItemPadding, -kNodeViewItemPadding, -kNodeViewItemPadding);
    painter->drawText(text_rect, Qt::AlignTop | Qt::AlignLeft, node_->Name());
  }
}
