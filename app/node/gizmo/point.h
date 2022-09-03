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

#ifndef POINTGIZMO_H
#define POINTGIZMO_H

#include <QPointF>

#include "draggable.h"

namespace olive {

class PointGizmo : public DraggableGizmo
{
  Q_OBJECT
public:
  enum Shape {
    kSquare,
    kCircle,
    kAnchorPoint
  };

  explicit PointGizmo(const Shape &shape, bool smaller, QObject *parent = nullptr);
  explicit PointGizmo(const Shape &shape, QObject *parent = nullptr);
  explicit PointGizmo(QObject *parent = nullptr);

  const Shape &GetShape() const { return shape_; }
  void SetShape(const Shape &s) { shape_ = s; }

  const QPointF &GetPoint() const { return point_; }
  void SetPoint(const QPointF &pt) { point_ = pt; }

  bool GetSmaller() const { return smaller_; }
  void SetSmaller(bool e) { smaller_ = e; }

  // avoid to move control points when multiple points are selected
  bool CanBeDraggedInGroup() const override {
    return smaller_ ? false : true;
  }

  virtual void Draw(QPainter *p) const override;

  QRectF GetClickingRect(const QTransform &t) const;

private:
  static double GetStandardRadius();

  QRectF GetDrawingRect(const QTransform &transform, double radius) const;

  Shape shape_;

  QPointF point_;

  bool smaller_;

};

}

#endif // POINTGIZMO_H
