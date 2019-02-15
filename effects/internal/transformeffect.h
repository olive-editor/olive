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

#include "project/effect.h"

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
	EffectField* position_x;
	EffectField* position_y;
	EffectField* scale_x;
	EffectField* scale_y;
	EffectField* uniform_scale_field;
	EffectField* rotation;
	EffectField* anchor_x_box;
	EffectField* anchor_y_box;
	EffectField* opacity;
	EffectField* blend_mode_box;

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

	bool set;
};

#endif // TRANSFORMEFFECT_H
