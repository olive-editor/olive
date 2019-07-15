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

#include <QFontMetrics>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "ui/icons/icons.h"

const int kNodeViewItemBorderWidth = 2;
const int kNodeViewItemWidth = 250;
const int kNodeViewItemTextPadding = 4;
const int kNodeViewItemIconPadding = 12;

NodeViewItem::NodeViewItem(QGraphicsItem *parent) :
  QGraphicsRectItem(parent),
  node_(nullptr)
{
  // Set flags for this widget
  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemIsSelectable);

  // Use the current default font height to size this widget
  QFont f;
  QFontMetrics fm(f);

  setRect(0, 0, kNodeViewItemWidth, fm.height() + kNodeViewItemTextPadding * 2);

  // FIXME: Magic "number"/magic "color" - allow this to be editable by the user
  SetColor(QColor(32, 32, 128));
}

void NodeViewItem::SetColor(const QColor &color)
{
  color_ = color;

  // Create a light gradient based on this color
  UpdateGradient();

  update();
}

void NodeViewItem::SetNode(Node *n)
{
  node_ = n;

  update();
}

Node *NodeViewItem::node()
{
  return node_;
}

void NodeViewItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  // Set up border, which will change color if selected
  QPen pen;

  pen.setWidth(kNodeViewItemBorderWidth);

  if (option->state & QStyle::State_Selected) {
    pen.setColor(widget->palette().highlight().color());
  } else {
    // FIXME: Not configurable?
    pen.setColor(Qt::black);
  }

  // Draw rect
  painter->setPen(pen);
  painter->setBrush(brush());
  painter->drawRect(rect());

  // Draw text
  if (node_ != nullptr) {
    // FIXME: The text is always drawn white assuming the color will be dark - the intention is to provide preset
    //        colors that will always be dark for the user to choose, so this value can stay white.
    painter->setPen(Qt::white);

    // Draw the expand icon
    QRectF icon_rect = rect();
    icon_rect.adjust(kNodeViewItemIconPadding,
                     kNodeViewItemIconPadding,
                     -kNodeViewItemIconPadding,
                     -kNodeViewItemIconPadding);
    olive::icon::TriRight.paint(painter, icon_rect.toRect(), Qt::AlignLeft | Qt::AlignVCenter);

    // Draw the text in a rect (the rect is sized around text already in the constructor)
    QRectF text_rect = rect();
    text_rect.adjust(kNodeViewItemIconPadding + icon_rect.height() + kNodeViewItemTextPadding,
                     kNodeViewItemTextPadding,
                     -kNodeViewItemTextPadding,
                     -kNodeViewItemTextPadding);
    painter->drawText(text_rect, Qt::AlignTop | Qt::AlignLeft, node_->Name());
  }
}

void NodeViewItem::UpdateGradient()
{
  QLinearGradient grad(QPointF(0, rect().top()), QPointF(0, rect().bottom()));
  grad.setColorAt(0, color_.lighter(175));
  grad.setColorAt(1, color_);
  setBrush(grad);
}
