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

#ifndef CORNERPINEFFECT_H
#define CORNERPINEFFECT_H

#include "project/effect.h"

class CornerPinEffect : public Effect {
	Q_OBJECT
public:
    CornerPinEffect(ClipPtr c, const EffectMeta* em);
	void process_coords(double timecode, GLTextureCoords& coords, int data);
	void process_shader(double timecode, GLTextureCoords& coords, int iterations);
	void gizmo_draw(double timecode, GLTextureCoords& coords);
private:
	EffectField* top_left_x;
	EffectField* top_left_y;
	EffectField* top_right_x;
	EffectField* top_right_y;
	EffectField* bottom_left_x;
	EffectField* bottom_left_y;
	EffectField* bottom_right_x;
	EffectField* bottom_right_y;
	EffectField* perspective;

	EffectGizmo* top_left_gizmo;
	EffectGizmo* top_right_gizmo;
	EffectGizmo* bottom_left_gizmo;
	EffectGizmo* bottom_right_gizmo;
};

#endif // CORNERPINEFFECT_H
