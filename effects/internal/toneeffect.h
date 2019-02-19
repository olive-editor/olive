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

#ifndef TONEEFFECT_H
#define TONEEFFECT_H

#include "project/effect.h"

class ToneEffect : public Effect {
    Q_OBJECT
public:
	ToneEffect(Clip *c, const EffectMeta* em);
	void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);

	EffectField* type_val;
	EffectField* freq_val;
	EffectField* amount_val;
	EffectField* mix_val;
private:
	int sinX;
};

#endif // TONEEFFECT_H
