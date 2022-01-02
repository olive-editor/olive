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

#include "nodeviewminimap.h"

#include <QMouseEvent>

namespace olive {

#define super QGraphicsView

NodeViewMiniMap::NodeViewMiniMap(NodeViewScene *scene, QWidget *parent) :
  super(parent),
  resizing_(false)
{
  connect(scene, &QGraphicsScene::sceneRectChanged, this, &NodeViewMiniMap::SceneChanged);
  setScene(scene);

  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setViewportUpdateMode(FullViewportUpdate);
  setFrameShape(QFrame::Panel);
  setFrameShadow(QFrame::Plain);
  setMouseTracking(true);

  QMetaObject::invokeMethod(this, &NodeViewMiniMap::SetDefaultSize, Qt::QueuedConnection);

  resize_triangle_sz_ = fontMetrics().height() / 2;
}

void NodeViewMiniMap::SetViewportRect(const QPolygonF &rect)
{
  viewport_rect_ = rect;

  viewport()->update();
}

void NodeViewMiniMap::drawForeground(QPainter *painter, const QRectF &rect)
{
  super::drawForeground(painter, rect);

  QColor viewport_color = palette().text().color();

  // Draw resize triangle
  painter->save();
  painter->resetTransform();

  QPointF triangle[3] = {QPointF(0, 0), QPointF(resize_triangle_sz_, 0), QPointF(0, resize_triangle_sz_)};
  painter->setBrush(viewport_color);
  painter->setPen(viewport_color);
  painter->drawPolygon(triangle, 3);

  painter->restore();

  // Draw viewport rectangle
  viewport_color.setAlphaF(0.25);
  painter->setBrush(viewport_color);

  painter->drawPolygon(viewport_rect_);
}

void NodeViewMiniMap::resizeEvent(QResizeEvent *event)
{
  super::resizeEvent(event);

  emit Resized();

  SceneChanged(sceneRect());
}

void NodeViewMiniMap::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton) {
    if (MouseInsideResizeTriangle(event)) {
      // Resizing!
      resizing_ = true;
      resize_anchor_ = QCursor::pos();
    } else {
      EmitMoveSignal(event);
    }
  }
}

void NodeViewMiniMap::mouseMoveEvent(QMouseEvent *event)
{
  if (event->buttons() & Qt::LeftButton) {
    if (resizing_) {
      QPointF movement = QCursor::pos() - resize_anchor_;
      resize(QSize(width() - movement.x(), height() - movement.y()));
      resize_anchor_ = QCursor::pos();
    } else {
      EmitMoveSignal(event);
    }
  } else {
    setCursor(MouseInsideResizeTriangle(event) ? Qt::SizeFDiagCursor : Qt::ArrowCursor);
  }
}

void NodeViewMiniMap::mouseReleaseEvent(QMouseEvent *event)
{
  resizing_ = false;
}

void NodeViewMiniMap::SceneChanged(const QRectF &bounding)
{
  double x_scale = double(this->width()) / bounding.width();
  double y_scale = double(this->height()) / bounding.height();

  double min_scale = qMin(x_scale, y_scale);

  QTransform transform;
  transform.scale(min_scale, min_scale);

  setTransform(transform);
}

void NodeViewMiniMap::SetDefaultSize()
{
  if (parentWidget()) {
    resize(parentWidget()->width()/4, parentWidget()->height()/4);
  }
}

bool NodeViewMiniMap::MouseInsideResizeTriangle(QMouseEvent *event)
{
  return event->pos().x() <= resize_triangle_sz_ && event->pos().y() <= resize_triangle_sz_;
}

void NodeViewMiniMap::EmitMoveSignal(QMouseEvent *event)
{
  emit MoveToScenePoint(mapToScene(event->pos()));
}

}
