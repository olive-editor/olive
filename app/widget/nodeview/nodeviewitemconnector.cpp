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

#include "nodeviewitemconnector.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPalette>
#include <QPen>

#include "nodeviewitem.h"

namespace olive {

NodeViewItemConnector::NodeViewItemConnector(bool is_output, QGraphicsItem *parent) :
  QGraphicsPolygonItem(parent),
  output_(is_output)
{
  QColor c = qApp->palette().text().color();
  setPen(QPen(c, NodeViewItem::DefaultItemBorder()));
  setBrush(c);
}

void NodeViewItemConnector::SetFlowDirection(NodeViewCommon::FlowDirection dir)
{
  QFont f;
  QFontMetricsF fm(f);

  int triangle_sz = fm.height()/2;
  int triangle_sz_half = triangle_sz / 2;

  QPolygonF p;
  p.resize(3);

  switch (dir) {
  case NodeViewCommon::kLeftToRight:
    // Triangle pointing right
    p[0] = QPointF(0, -triangle_sz_half);
    p[1] = QPointF(triangle_sz_half, 0);
    p[2] = QPointF(0, triangle_sz_half);
    break;
  case NodeViewCommon::kTopToBottom:
    // Triangle pointing down
    p[0] = QPointF(-triangle_sz_half, 0);
    p[1] = QPointF(0, triangle_sz_half);
    p[2] = QPointF(triangle_sz_half, 0);
    break;
  case NodeViewCommon::kBottomToTop:
    // Triangle pointing up
    p[0] = QPointF(-triangle_sz_half, 0);
    p[1] = QPointF(0, -triangle_sz_half);
    p[2] = QPointF(triangle_sz_half, 0);
    break;
  case NodeViewCommon::kRightToLeft:
    // Triangle pointing left
    p[0] = QPointF(0, -triangle_sz_half);
    p[1] = QPointF(-triangle_sz_half, 0);
    p[2] = QPointF(0, triangle_sz_half);
    break;
  case NodeViewCommon::kInvalidDirection:
    break;
  }

  setPolygon(p);
}

}
