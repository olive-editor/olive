#include "paneffect.h"

#include <QGridLayout>
#include <QLabel>
#include <QtMath>
#include <cmath>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"

PanEffect::PanEffect(Clip* c, const EffectMeta *em) : Effect(c, em) {
	EffectRow* pan_row = add_row("Pan:");
	pan_val = pan_row->add_field(EFFECT_FIELD_DOUBLE);
	pan_val->set_double_minimum_value(-100);
	pan_val->set_double_maximum_value(100);
	pan_val->id = "pan";

	// set defaults
	pan_val->set_double_default_value(0);

	connect(pan_val, SIGNAL(changed()), this, SLOT(field_changed()));
}

void PanEffect::process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int) {
	double interval = (timecode_end - timecode_start)/nb_bytes;
	for (int i=0;i<nb_bytes;i+=4) {
		double pval = log_volume(pan_val->get_double_value(timecode_start+(interval*i), true)*0.01);
		qint16 left_sample = (qint16) (((samples[i+1] & 0xFF) << 8) | (samples[i] & 0xFF));
		qint16 right_sample = (qint16) (((samples[i+3] & 0xFF) << 8) | (samples[i+2] & 0xFF));

		if (pval < 0) {
			// affect right channel
			right_sample *= (1-std::abs(pval));
		} else {
			// affect left channel
			left_sample *= (1-pval);
		}

		samples[i+3] = (quint8) (right_sample >> 8);
		samples[i+2] = (quint8) right_sample;
		samples[i+1] = (quint8) (left_sample >> 8);
		samples[i] = (quint8) left_sample;
	}
}
