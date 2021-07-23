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

#include "nodeviewedge.h"

#include <QApplication>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>

#include "common/bezier.h"
#include "common/lerp.h"
#include "nodeview.h"
#include "nodeviewitem.h"
#include "nodeviewscene.h"

namespace olive {

#define super QGraphicsPathItem

NodeViewEdge::NodeViewEdge(const NodeOutput &output, const NodeInput &input,
                           NodeViewItem* from_item, NodeViewItem* to_item,
                           QGraphicsItem* parent) :
  super(parent),
  output_(output),
  input_(input),
  from_item_(from_item),
  to_item_(to_item)
{
  Init();
  SetConnected(true);
}

NodeViewEdge::NodeViewEdge(QGraphicsItem *parent) :
  QGraphicsPathItem(parent),
  from_item_(nullptr),
  to_item_(nullptr)
{
  Init();
}

void NodeViewEdge::Adjust()
{
  // Draw a line between the two
  SetPoints(from_item()->GetOutputPoint(output_.output()),
            to_item()->GetInputPoint(input_.input(), input_.element()));
}

void NodeViewEdge::SetConnected(bool c)
{
  connected_ = c;

  update();
}

void NodeViewEdge::SetHighlighted(bool e)
{
  highlighted_ = e;

  update();
}

void NodeViewEdge::SetPoints(const QPointF &start, const QPointF &end)
{
  cached_start_ = start;
  cached_end_ = end;

  UpdateCurve();
}

void NodeViewEdge::SetFlowDirection(NodeViewCommon::FlowDirection dir)
{
  flow_dir_ = dir;

  if (from_item_ && to_item_) {
    Adjust();
  }
}

void NodeViewEdge::SetCurved(bool e)
{
  curved_ = e;

  UpdateCurve();
}

void NodeViewEdge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
  QPalette::ColorGroup group;
  QPalette::ColorRole role;

  if (connected_) {
    group = QPalette::Active;
  } else {
    group = QPalette::Disabled;
  }

  if (highlighted_ != bool(option->state & QStyle::State_Selected)) {
    role = QPalette::Highlight;
  } else {
    role = QPalette::Text;
  }

  // Draw main path
  QColor edge_color = qApp->palette().color(group, role);

  painter->setPen(QPen(edge_color, edge_width_));
  painter->setBrush(Qt::NoBrush);
  painter->drawPath(path());
}

void NodeViewEdge::Init()
{
  connected_ = false;
  highlighted_ = false;
  flow_dir_ = NodeViewCommon::kLeftToRight;
  curved_ = true;

  setFlag(QGraphicsItem::ItemIsSelectable);

  // Ensures this UI object is drawn behind other objects
  setZValue(-1);

  // Use font metrics to set edge width for basic high DPI support
  edge_width_ = QFontMetrics(QFont()).height() / 12;
}

void NodeViewEdge::UpdateCurve()
{
  const QPointF &start = cached_start_;
  const QPointF &end = cached_end_;

  QPainterPath path;
  path.moveTo(start);

  if (curved_) {

    double half_x = lerp(start.x(), end.x(), 0.5);
    double half_y = lerp(start.y(), end.y(), 0.5);

    QPointF cp1, cp2;

    if (NodeViewCommon::GetFlowOrientation(flow_dir_) == Qt::Horizontal) {
      cp1 = QPointF(half_x, start.y());
    } else {
      cp1 = QPointF(start.x(), half_y);
    }

    if (NodeViewCommon::GetFlowOrientation(flow_dir_) == Qt::Horizontal) {
      cp2 = QPointF(half_x, end.y());
    } else {
      cp2 = QPointF(end.x(), half_y);
    }

    path.cubicTo(cp1, cp2, end);

  } else {

    path.lineTo(end);

  }

  setPath(path);
}

}
