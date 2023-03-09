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

#include "line.h"

namespace olive {

LineGizmo::LineGizmo(QObject *parent) :
  NodeGizmo(parent)
{
}

void LineGizmo::Draw(QPainter *p) const
{
  // Draw transposed black
  QLineF transposed = p->transform().map(line_);
  transposed.translate(1, 1);
  transposed = p->transform().inverted().map(transposed);
  p->setPen(QPen(Qt::black, 0));
  p->drawLine(transposed);

  // Draw normal white polygon
  p->setPen(QPen(Qt::white, 0));
  p->setBrush(Qt::NoBrush);
  p->drawLine(line_);
}

}
