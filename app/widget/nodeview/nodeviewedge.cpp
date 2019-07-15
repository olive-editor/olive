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

#include "clamp.h"
#include "lerp.h"
#include "nodeview.h"
#include "nodeviewitem.h"

NodeViewEdge::NodeViewEdge(QGraphicsItem *parent) :
  QGraphicsLineItem(parent),
  edge_(nullptr)
{
  // FIXME: This should probably be set to the text color in order to work on light themes
  setPen(QPen(Qt::white, 2));

  // Ensures this UI object is drawn behind other objects
  setZValue(-1);
}

void NodeViewEdge::SetEdge(NodeEdgePtr edge)
{
  // Set the new edge pointer
  edge_ = edge;

  // Re-adjust the line positioning for this new edge
  Adjust();
}

void NodeViewEdge::Adjust()
{
  if (edge_ == nullptr || scene() == nullptr) {
    return;
  }

  // Get the UI objects of both nodes that this edge connects
  NodeViewItem* output = NodeView::NodeToUIObject(scene(), edge_->output()->parent());
  NodeViewItem* input = NodeView::NodeToUIObject(scene(), edge_->input()->parent());

  // Calculate output/input points
  qreal value = clamp(0.5 + (output->pos().y() - input->pos().y()) / (input->rect().height()) / 4, 0.0, 1.0);

  // Use a lerp function to draw the line between the two corners
  qreal output_point = output->pos().y() + lerp(0.0, output->rect().height(), 1.0 - value);
  qreal input_point = input->pos().y() + lerp(0.0, input->rect().height(), value);

  // Draw a line between the two
  setLine(QLineF(
            QPointF(output->pos().x() + output->rect().width(), output_point),
            QPointF(input->pos().x(), input_point)
            ));
}
