/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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
#include "common/clamp.h"
#include "common/lerp.h"
#include "nodeview.h"
#include "nodeviewscene.h"

namespace olive {

NodeViewEdge::NodeViewEdge(QGraphicsItem *parent) :
  QGraphicsPathItem(parent),
  edge_(nullptr),
  connected_(false),
  highlighted_(false),
  flow_dir_(NodeViewCommon::kLeftToRight),
  curved_(true)
{
  setFlag(QGraphicsItem::ItemIsSelectable);

  // Ensures this UI object is drawn behind other objects
  setZValue(-1);

  // Use font metrics to set edge width for basic high DPI support
  edge_width_ = QFontMetrics(QFont()).height() / 12;
  arrow_size_ = QFontMetrics(QFont()).height() / 2;
}

void NodeViewEdge::SetEdge(NodeEdgePtr edge)
{
  SetConnected(true);

  // Set the new edge pointer
  edge_ = edge;

  // Re-adjust the line positioning for this new edge
  Adjust();
}

NodeEdgePtr NodeViewEdge::edge()
{
  return edge_;
}

void NodeViewEdge::Adjust()
{
  if (!edge_ || !scene()) {
    return;
  }

  // Get the UI objects of both nodes that this edge connects
  NodeViewItem* output = static_cast<NodeViewScene*>(scene())->NodeToUIObject(edge_->output()->parentNode());
  NodeViewItem* input = static_cast<NodeViewScene*>(scene())->NodeToUIObject(edge_->input()->parentNode());

  if (!output || !input) {
    return;
  }

  // Draw a line between the two
  SetPoints(output->GetParamPoint(edge_->output(), output->pos()),
            input->GetParamPoint(edge_->input(), output->pos()),
            input->IsExpanded());
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

      double x1, x2, x3, x4, y1, y2, y3, y4;
      if (start.x() < end.x()) {
        x1 = start.x();
        x2 = cp1.x();
        x3 = cp2.x();
        x4 = end.x();
        y1 = start.y();
        y2 = cp1.y();
        y3 = cp2.y();
        y4 = end.y();
      } else {
        x1 = end.x();
        x2 = cp2.x();
        x3 = cp1.x();
        x4 = start.x();
        y1 = end.y();
        y2 = cp2.y();
        y3 = cp1.y();
        y4 = start.y();
      }

      double t = Bezier::CubicXtoT(continue_x, x1, x2, x3, x4);
      double y = Bezier::CubicTtoY(y1, y2, y3, y4, t);

      angle = qAtan2(end.y() - y, end.x() - continue_x);
    }

  } else {

    path.lineTo(end);

  }

  setPath(path);

  const double arrow_angle = 150.0 * 3.141592 / 180.0;
  QVector<QPointF> arrow_points(4);
  arrow_points[0] = end;
  arrow_points[1] = end + QPointF(qCos(angle + arrow_angle) * arrow_size_, qSin(angle + arrow_angle) * arrow_size_);
  arrow_points[2] = end + QPointF(qCos(angle - arrow_angle) * arrow_size_, qSin(angle - arrow_angle) * arrow_size_);
  arrow_points[3] = end;

  arrow_ = QPolygonF(arrow_points);
}

void NodeViewEdge::SetFlowDirection(NodeViewCommon::FlowDirection dir)
{
  flow_dir_ = dir;

  Adjust();
}

void NodeViewEdge::SetCurved(bool e)
{
  curved_ = e;

  update();
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
  painter->setPen(Qt::NoPen);
  painter->setBrush(edge_color);
  painter->drawPolygon(arrow_);
}

}
