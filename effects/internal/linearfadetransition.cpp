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

#include "linearfadetransition.h"

LinearFadeTransition::LinearFadeTransition(ClipPtr c, ClipPtr s, const EffectMeta* em) : Transition(c, s, em) {}

void LinearFadeTransition::process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int type) {
	double interval = (timecode_end-timecode_start)/nb_bytes;

	for (int i=0;i<nb_bytes;i+=2) {
		qint16 samp = (qint16) (((samples[i+1] & 0xFF) << 8) | (samples[i] & 0xFF));

		switch (type) {
        case TA_OPENING_TRANSITION:
			samp *= timecode_start + (interval * i);
			break;
        case TA_CLOSING_TRANSITION:
			samp *= 1 - (timecode_start + (interval * i));
			break;
		}

		samples[i+1] = (quint8) (samp >> 8);
		samples[i] = (quint8) samp;
	}
}
