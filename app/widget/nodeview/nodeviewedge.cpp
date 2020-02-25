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

#include "nodeviewedge.h"

#include <QDebug>
#include <QGraphicsSceneMouseEvent>

#include "common/clamp.h"
#include "common/lerp.h"
#include "nodeview.h"

NodeViewEdge::NodeViewEdge(QGraphicsItem *parent) :
  QGraphicsLineItem(parent),
  edge_(nullptr),
  connected_(false),
  highlighted_(false)
{
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

qreal CalculateEdgeYPoint(NodeViewItem *item, NodeParam* param, NodeViewItem *opposing)
{
  if (item->IsExpanded()) {
    return item->pos().y() + item->GetParameterConnectorRect(param).center().y();
  } else {
    qreal max_height = qMax(opposing->rect().height(), item->rect().height());

    // Calculate the Y distance between the two nodes and create a 0.0-1.0 range for lerping
    qreal input_value = clamp(0.5 + ((opposing->pos().y() + opposing->rect().top()) - (item->pos().y() + item->rect().top())) / max_height / 4, 0.0, 1.0);

    // Use a lerp function to draw the line between the two corners
    qreal input_y = item->pos().y() + item->rect().top() + lerp(0.0, item->rect().height(), input_value);

    // Set Y values according to calculations
    return input_y;
  }
}

void NodeViewEdge::Adjust()
{
  if (edge_ == nullptr || scene() == nullptr) {
    return;
  }

  // Get the UI objects of both nodes that this edge connects
  NodeViewItem* output = NodeView::NodeToUIObject(scene(), edge_->output()->parentNode());
  NodeViewItem* input = NodeView::NodeToUIObject(scene(), edge_->input()->parentNode());

  // Create initial values
  QPointF output_point = QPointF(output->pos().x() + output->rect().left() + output->rect().width(), 0);
  QPointF input_point = QPointF(input->pos().x() + output->rect().left(), 0);

  // Calculate output/input points
  output_point.setY(CalculateEdgeYPoint(output, edge_->output(), input));
  input_point.setY(CalculateEdgeYPoint(input, edge_->input(), output));

  // Draw a line between the two
  setLine(QLineF(
            output_point,
            input_point
            ));
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

void NodeViewEdge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  QPalette::ColorGroup color_group = QPalette::Active;
  QPalette::ColorRole color_role = QPalette::Text;

  if (highlighted_) {
    color_role = QPalette::Highlight;
  }

  if (!connected_) {
    color_group = QPalette::Disabled;
  }

  setPen(QPen(widget->palette().color(color_group, color_role), edge_width_));

  QGraphicsLineItem::paint(painter, option, widget);
}

