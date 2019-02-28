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

#include "paneffect.h"

#include <QGridLayout>
#include <QLabel>
#include <QtMath>
#include <cmath>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"

PanEffect::PanEffect(Clip* c, const EffectMeta *em) : Effect(c, em) {
	EffectRow* pan_row = add_row(tr("Pan"));
	pan_val = pan_row->add_field(EFFECT_FIELD_DOUBLE, "pan");
	pan_val->set_double_minimum_value(-100);
	pan_val->set_double_maximum_value(100);

	// set defaults
	pan_val->set_double_default_value(0);
}

void PanEffect::process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int) {
	double interval = (timecode_end - timecode_start)/nb_bytes;
	for (int i=0;i<nb_bytes;i+=4) {
		double pan_field_val = pan_val->get_double_value(timecode_start+(interval*i), true);
		double pval = log_volume(qAbs(pan_field_val)*0.01);

		qint16 left_sample = qint16(((samples[i+1] & 0xFF) << 8) | (samples[i] & 0xFF));
		qint16 right_sample = qint16(((samples[i+3] & 0xFF) << 8) | (samples[i+2] & 0xFF));

		if (pan_field_val < 0) {
			// affect right channel
			right_sample *= (1.0-pval);
		} else {
			// affect left channel
			left_sample *= (1.0-pval);
		}

		samples[i+3] = quint8(right_sample >> 8);
		samples[i+2] = quint8(right_sample);
		samples[i+1] = quint8(left_sample >> 8);
		samples[i] = quint8(left_sample);
	}
}
