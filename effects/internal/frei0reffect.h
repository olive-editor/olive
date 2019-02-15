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

#ifndef FREI0REFFECT_H
#define FREI0REFFECT_H

#ifndef NOFREI0R

#include "project/effect.h"

#include <frei0r.h>

#include "io/crossplatformlib.h"

typedef void (*f0rGetParamInfo)(f0r_param_info_t * info,
								int param_index );

class Frei0rEffect : public Effect {
	Q_OBJECT
public:
	Frei0rEffect(Clip* c, const EffectMeta* em);
	~Frei0rEffect();

	virtual void process_image(double timecode, uint8_t* input, uint8_t* output, int size);

	virtual void refresh();
private:
	ModulePtr handle;
	f0r_instance_t instance;
	int param_count;
	f0rGetParamInfo get_param_info;
	void destruct_module();
	void construct_module();
	bool open;
};

#endif

#endif // FREI0REFFECT_H
