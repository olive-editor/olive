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

#include "nodeview.h"

#include <QGraphicsRectItem>

NodeView::NodeView(QWidget *parent) :
  QGraphicsView(parent),
  graph_(nullptr)
{
  setScene(&scene_);

  QGraphicsRectItem* rect = scene_.addRect(0, 0, 50, 50, QPen(Qt::red), Qt::blue);

  QGraphicsTextItem* text = scene_.addText("text");

  QGraphicsItemGroup* group = scene_.createItemGroup({rect, text});
  group->setFlag(QGraphicsItem::ItemIsMovable);
}

void NodeView::SetGraph(NodeGraph *graph)
{
  graph_ = graph;
}
