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

  } else {

    path.lineTo(end);

  }

  setPath(path);
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

  painter->setPen(QPen(qApp->palette().color(group, role), edge_width_));
  painter->setBrush(Qt::NoBrush);
  painter->drawPath(path());
}

}
