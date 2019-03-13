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

#ifndef TRANSFORMEFFECT_H
#define TRANSFORMEFFECT_H

#include "effects/effect.h"

class TransformEffect : public Effect {
  Q_OBJECT
public:
    TransformEffect(Clip* c, const EffectMeta* em);
  void refresh();
  void process_coords(double timecode, GLTextureCoords& coords, int data);

  void gizmo_draw(double timecode, GLTextureCoords& coords);
public slots:
  void toggle_uniform_scale(bool enabled);
private:
  DoubleField* position_x;
  DoubleField* position_y;
  DoubleField* scale_x;
  DoubleField* scale_y;
  BoolField* uniform_scale_field;
  DoubleField* rotation;
  DoubleField* anchor_x_box;
  DoubleField* anchor_y_box;
  DoubleField* opacity;
  ComboField* blend_mode_box;

  EffectGizmo* top_left_gizmo;
  EffectGizmo* top_center_gizmo;
  EffectGizmo* top_right_gizmo;
  EffectGizmo* bottom_left_gizmo;
  EffectGizmo* bottom_center_gizmo;
  EffectGizmo* bottom_right_gizmo;
  EffectGizmo* left_center_gizmo;
  EffectGizmo* right_center_gizmo;
  EffectGizmo* anchor_gizmo;
  EffectGizmo* rotate_gizmo;
  EffectGizmo* rect_gizmo;
};

#endif // TRANSFORMEFFECT_H
