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
  to_item_(to_item),
  draw_arrow_(true)
{
  Init();
  SetConnected(true);
}

NodeViewEdge::NodeViewEdge(QGraphicsItem *parent) :
  QGraphicsPathItem(parent),
  from_item_(nullptr),
  to_item_(nullptr),
  draw_arrow_(false)
{
  Init();
}

void NodeViewEdge::Adjust()
{
  // Draw a line between the two
  SetPoints(from_item()->GetOutputPoint(output_.output()),
            to_item()->GetInputPoint(input_.input(), input_.element(), from_item()->pos()),
            to_item()->IsExpanded());
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

void NodeViewEdge::SetPoints(const QPointF &start, const QPointF &end, bool input_is_expanded)
{
  cached_start_ = start;
  cached_end_ = end;
  cached_input_is_expanded_ = input_is_expanded;

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

  // Draw arrow
  if (DrawArrow()) {
    painter->setPen(Qt::NoPen);
    painter->setBrush(edge_color);
    painter->drawPolygon(arrow_);
  }
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
  arrow_size_ = QFontMetrics(QFont()).height() / 2;
}

void NodeViewEdge::UpdateCurve()
{
  const QPointF &start = cached_start_;
  const QPointF &end = cached_end_;
  const bool input_is_expanded = cached_input_is_expanded_;

  QPainterPath path;
  path.moveTo(start);

  double angle = qAtan2(end.y() - start.y(), end.x() - start.x());

  if (curved_) {

    double half_x = lerp(start.x(), end.x(), 0.5);
    double half_y = lerp(start.y(), end.y(), 0.5);

    QPointF cp1, cp2;

    if (NodeViewCommon::GetFlowOrientation(flow_dir_) == Qt::Horizontal) {
      cp1 = QPointF(half_x, start.y());
    } else {
      cp1 = QPointF(start.x(), half_y);
    }

    if (NodeViewCommon::GetFlowOrientation(flow_dir_) == Qt::Horizontal || input_is_expanded) {
      cp2 = QPointF(half_x, end.y());
    } else {
      cp2 = QPointF(end.x(), half_y);
    }

    path.cubicTo(cp1, cp2, end);

    if (!qFuzzyCompare(start.x(), end.x())) {
      double continue_x = end.x() - qCos(angle)*arrow_size_;

      double x1 = start.x();
      double x2 = cp1.x();
      double x3 = cp2.x();
      double x4 = end.x();
      double y1 = start.y();
      double y2 = cp1.y();
      double y3 = cp2.y();
      double y4 = end.y();

      if (start.x() >= end.x()) {
        std::swap(x1, x4);
        std::swap(x2, x3);
        std::swap(y1, y4);
        std::swap(y2, y3);
      }

      double t = Bezier::CubicXtoT(continue_x, x1, x2, x3, x4);
      double y = Bezier::CubicTtoY(y1, y2, y3, y4, t);

      angle = qAtan2(end.y() - y, end.x() - continue_x);
    }

  } else {

    path.lineTo(end);

  }

  setPath(path);

  const double arrow_angle = 150.0 * M_PI / 180.0;
  QVector<QPointF> arrow_points(4);
  arrow_points[0] = end;
  arrow_points[1] = end + QPointF(qCos(angle + arrow_angle) * arrow_size_, qSin(angle + arrow_angle) * arrow_size_);
  arrow_points[2] = end + QPointF(qCos(angle - arrow_angle) * arrow_size_, qSin(angle - arrow_angle) * arrow_size_);
  arrow_points[3] = end;

  arrow_ = QPolygonF(arrow_points);
  arrow_bounding_rect_ = arrow_.boundingRect();
  arrow_bounding_rect_.adjust(-arrow_size_, -arrow_size_, arrow_size_, arrow_size_);
}

}
