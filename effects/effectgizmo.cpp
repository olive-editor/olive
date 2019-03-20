/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "effectgizmo.h"

#include "ui/labelslider.h"
#include "effects/fields/doublefield.h"
#include "effects/effect.h"

EffectGizmo::EffectGizmo(Effect *parent, int type) :
  QObject(parent),
  x_field1(nullptr),
  x_field_multi1(1.0),
  y_field1(nullptr),
  y_field_multi1(1.0),
  x_field2(nullptr),
  x_field_multi2(1.0),
  y_field2(nullptr),
  y_field_multi2(1.0),
  type(type),
  cursor(-1)
{
  Q_ASSERT(parent != nullptr);

  int point_count = (type == GIZMO_TYPE_POLY) ? 4 : 1;
  world_pos.resize(point_count);
  screen_pos.resize(point_count);

  color = Qt::white;
}

int EffectGizmo::get_point_count() {
  return world_pos.size();
}

int EffectGizmo::get_type() {
  return type;
}

int EffectGizmo::get_cursor() {
  return cursor;
}

void EffectGizmo::set_cursor(int c) {
  cursor = c;
}
