/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "nodeviewconnector.h"

#include "core.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

NodeViewConnector::NodeViewConnector(QGraphicsItem *parent) :
  QGraphicsItem(parent)
{
  int triangle_sz = QFontMetrics(QFont()).height()/2;

  rect_ = QRectF(-triangle_sz/2, -triangle_sz, triangle_sz, triangle_sz);
}

void NodeViewConnector::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  // Use main window palette since the palette passed in `widget` is the NodeView palette which
  // has been slightly modified
  QPalette app_pal = Core::instance()->main_window()->palette();

  // Draw output triangle
  painter->setPen(Qt::NoPen);
  painter->setBrush(app_pal.color(QPalette::Text));

  QVector<QPointF> triangle(3);

  switch (flow_dir_) {
  case NodeViewCommon::kLeftToRight:
    // Triangle pointing right
    triangle[0] = rect_.topLeft();
    triangle[1] = QPointF(rect_.right(), rect_.center().y());
    triangle[2] = rect_.bottomLeft();
    break;
  case NodeViewCommon::kTopToBottom:
    // Triangle pointing down
    triangle[0] = rect_.topLeft();
    triangle[1] = QPointF(rect_.center().x(), rect_.bottom());
    triangle[2] = rect_.topRight();
    break;
  case NodeViewCommon::kBottomToTop:
    // Triangle pointing up
    triangle[0] = rect_.bottomLeft();
    triangle[1] = QPointF(rect_.center().x(), rect_.top());
    triangle[2] = rect_.bottomRight();
    break;
  case NodeViewCommon::kRightToLeft:
    // Triangle pointing left
    triangle[0] = rect_.topRight();
    triangle[1] = QPointF(rect_.left(), rect_.center().y());
    triangle[2] = rect_.bottomRight();
    break;
  }

  painter->drawPolygon(triangle);
}

}
