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

#include "point.h"

#include <QApplication>

namespace olive {

PointGizmo::PointGizmo(const Shape &shape, bool smaller, QObject *parent) :
  DraggableGizmo{parent},
  shape_(shape),
  smaller_(smaller)
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
  QRectF rect = GetDrawingRect(GetStandardRadius() / p->transform().m11());

  if (shape_ != kAnchorPoint) {
    p->setPen(Qt::NoPen);
    p->setBrush(Qt::white);
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
  return GetDrawingRect(GetStandardRadius() / t.m11() * 1.5);
}

double PointGizmo::GetStandardRadius()
{
  return QFontMetrics(qApp->font()).height() * 0.25;
}

QRectF PointGizmo::GetDrawingRect(double radius) const
{
  if (shape_ == kAnchorPoint) {
    radius *= 2;
  }

  if (smaller_) {
    radius *= 0.5;
  }

  return QRectF(point_.x() - radius,
                point_.y() - radius,
                2*radius,
                2*radius);
}

}
