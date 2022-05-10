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

#ifndef PATHGIZMO_H
#define PATHGIZMO_H

#include <QPainterPath>

#include "draggable.h"

namespace olive {

class PathGizmo : public DraggableGizmo
{
  Q_OBJECT
public:
  explicit PathGizmo(QObject *parent = nullptr);

  const QPainterPath &GetPath() const { return path_; }
  void SetPath(const QPainterPath &path) { path_ = path; }

  virtual void Draw(QPainter *p) const override;

private:
  QPainterPath path_;

};

}

#endif // PATHGIZMO_H
