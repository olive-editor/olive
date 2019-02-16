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

#include "fillleftrighteffect.h"

#define FILL_TYPE_LEFT 0
#define FILL_TYPE_RIGHT 1

FillLeftRightEffect::FillLeftRightEffect(Clip* c, const EffectMeta *em) : Effect(c, em) {
    EffectRow* type_row = add_row(tr("Type"));
	fill_type = type_row->add_field(EFFECT_FIELD_COMBO, "type");
    fill_type->add_combo_item(tr("Fill Left with Right"), FILL_TYPE_LEFT);
    fill_type->add_combo_item(tr("Fill Right with Left"), FILL_TYPE_RIGHT);
}

void FillLeftRightEffect::process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int) {
	double interval = (timecode_end-timecode_start)/nb_bytes;
	for (int i=0;i<nb_bytes;i+=4) {
		if (fill_type->get_combo_data(timecode_start+(interval*i)) == FILL_TYPE_LEFT) {
			samples[i+1] = samples[i+3];
			samples[i] = samples[i+2];
		} else {
			samples[i+3] = samples[i+1];
			samples[i+2] = samples[i];
		}
	}
}
