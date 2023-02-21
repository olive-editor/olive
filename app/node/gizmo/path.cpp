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

#include "path.h"

namespace olive {

PathGizmo::PathGizmo(QObject *parent) :
  DraggableGizmo{parent}
{

}

void PathGizmo::Draw(QPainter *p) const
{
  // Draw transposed black
  QPainterPath transposed = p->transform().map(path_);
  transposed.translate(1, 1);
  transposed = p->transform().inverted().map(transposed);
  p->setPen(QPen(Qt::black, 0));
  p->drawPath(transposed);

  // Draw normal white polygon
  p->setPen(QPen(Qt::white, 0));
  p->setBrush(Qt::NoBrush);
  p->drawPath(path_);
}

}
