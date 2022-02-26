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

NodeViewEdge::NodeViewEdge(Node *output, const NodeInput &input,
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

  from_item_->AddEdge(this);
  to_item_->AddEdge(this);
}

NodeViewEdge::NodeViewEdge(QGraphicsItem *parent) :
  QGraphicsPathItem(parent),
  from_item_(nullptr),
  to_item_(nullptr)
{
  Init();
}

NodeViewEdge::~NodeViewEdge()
{
  if (from_item_) {
    from_item_->RemoveEdge(this);
  }

  if (to_item_) {
    to_item_->RemoveEdge(this);
  }
}

void NodeViewEdge::set_from_item(NodeViewItem *i)
{
  if (from_item_) {
    from_item_->RemoveEdge(this);
  }

  from_item_ = i;

  if (from_item_) {
    from_item_->AddEdge(this);
  }

  Adjust();
}

void NodeViewEdge::set_to_item(NodeViewItem *i)
{
  if (to_item_) {
    to_item_->RemoveEdge(this);
  }

  to_item_ = i;

  if (to_item_) {
    to_item_->AddEdge(this);
  }

  Adjust();
}

void NodeViewEdge::Adjust()
{
  // Draw a line between the two
  SetPoints(from_item()->GetOutputPoint(), to_item()->GetInputPoint());
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

  double angle = qAtan2(end.y() - start.y(), end.x() - start.x());

  if (curved_) {

    double half_x = lerp(start.x(), end.x(), 0.5);
    double half_y = lerp(start.y(), end.y(), 0.5);

    QPointF cp1, cp2;

    NodeViewCommon::FlowDirection from_flow = from_item_ ? from_item_->GetFlowDirection() : NodeViewCommon::kInvalidDirection;
    NodeViewCommon::FlowDirection to_flow = to_item_ ? to_item_->GetFlowDirection() : NodeViewCommon::kInvalidDirection;

    if (from_flow == NodeViewCommon::kInvalidDirection && to_flow == NodeViewCommon::kInvalidDirection) {
      // This is a technically unsupported scenario, but to avoid issues, we'll use a fallback
      from_flow = NodeViewCommon::kLeftToRight;
      to_flow = NodeViewCommon::kLeftToRight;
    } else if (from_flow == NodeViewCommon::kInvalidDirection) {
      from_flow = to_flow;
    } else if (to_flow == NodeViewCommon::kInvalidDirection) {
      to_flow = from_flow;
    }

    if (NodeViewCommon::GetFlowOrientation(from_flow) == Qt::Horizontal) {
      cp1 = QPointF(half_x, start.y());
    } else {
      cp1 = QPointF(start.x(), half_y);
    }

    if (NodeViewCommon::GetFlowOrientation(to_flow) == Qt::Horizontal) {
      cp2 = QPointF(half_x, end.y());
    } else {
      cp2 = QPointF(end.x(), half_y);
    }

    path.cubicTo(cp1, cp2, end);

    if (!qFuzzyCompare(start.x(), end.x())) {
      double continue_x = end.x() - qCos(angle);

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

  setPath(mapFromScene(path));
}

}
