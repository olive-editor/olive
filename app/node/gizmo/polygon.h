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

#ifndef POLYGONGIZMO_H
#define POLYGONGIZMO_H

#include <QPolygonF>

#include "draggable.h"

namespace olive {

class PolygonGizmo : public DraggableGizmo
{
  Q_OBJECT
public:
  explicit PolygonGizmo(QObject *parent = nullptr);

  const QPolygonF &GetPolygon() const { return polygon_; }
  void SetPolygon(const QPolygonF &polygon) { polygon_ = polygon; }

  virtual void Draw(QPainter *p) const override;

private:
  QPolygonF polygon_;

};

}

#endif // POLYGONGIZMO_H
