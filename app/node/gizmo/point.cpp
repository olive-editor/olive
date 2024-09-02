/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "point.h"

#include <QApplication>

namespace olive {

PointGizmo::PointGizmo(const Shape &shape, bool smaller, QObject *parent) :
  DraggableGizmo{parent},
  shape_(shape),
  smaller_(smaller),
  can_drag_in_group_(false)
{
}

PointGizmo::PointGizmo(const Shape &shape, QObject *parent) :
  PointGizmo(shape, false, parent)
{
}

PointGizmo::PointGizmo(QObject *parent) :
  PointGizmo(kSquare, parent)
{
}

void PointGizmo::Draw(QPainter *p) const
{
  QRectF rect = GetDrawingRect(p->transform(), GetStandardRadius());

  if (shape_ != kAnchorPoint) {
    p->setBrush( IsHovered() ? QColor(0x20,0xFF, 0xFF) : (IsSelected() ? QColor(0xCC,0x00, 0xFF) : Qt::white));
    p->setPen( IsHovered() ? QPen(QColor(0x10,0x80, 0x80),6) :
                             (IsSelected() ? QPen(QColor(0x66,0x00, 0x80), 4) : QPen(Qt::white, 4)));
  }

  switch (shape_) {
  case kSquare:
    p->drawRect(rect);
    break;
  case kCircle:
    p->drawEllipse(rect);
    break;
  case kAnchorPoint:
    p->setPen(QPen(Qt::white, 0));
    p->setBrush(Qt::NoBrush);

    p->drawEllipse(rect);
    p->drawLines({QPointF(rect.left(), rect.center().y()),
                  QPointF(rect.right(), rect.center().y()),
                  QPointF(rect.center().x(), rect.top()),
                  QPointF(rect.center().x(), rect.bottom())});
    break;
  }
}

QRectF PointGizmo::GetClickingRect(const QTransform &t) const
{
  // clicking rect is a little bigger than the painted rect
  return GetDrawingRect(t, GetStandardRadius()*2.2);
}

double PointGizmo::GetStandardRadius()
{
  return QFontMetrics(qApp->font()).height() * 0.3;
}

QRectF PointGizmo::GetDrawingRect(const QTransform &transform, double radius) const
{
  QRectF r(0,0, radius, radius);

  r = transform.inverted().mapRect(r);

  double width = r.width();
  double height = r.height();

  if (shape_ == kAnchorPoint) {
    width *= 2;
    height *= 2;
  }

  if (smaller_) {
    width *= 0.6;
    height *= 0.6;
  }

  return QRectF(point_.x() - width,
                point_.y() - height,
                2*width,
                2*height);
}

}
