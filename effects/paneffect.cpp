#include "effects/effects.h"

#include <QGridLayout>
#include <QLabel>
#include <QtMath>
#include <cmath>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"

PanEffect::PanEffect(Clip* c) : Effect(c, EFFECT_TYPE_AUDIO, AUDIO_PAN_EFFECT) {
	EffectRow* pan_row = add_row("Pan:");
	pan_val = pan_row->add_field(EFFECT_FIELD_DOUBLE);
	pan_val->set_double_minimum_value(-100);
	pan_val->set_double_maximum_value(100);

	// set defaults
	pan_val->set_double_default_value(0);
}

void PanEffect::refresh() {}

Effect* PanEffect::copy(Clip* c) {
	/*PanEffect* p = new PanEffect(c);
    p->pan_val->set_value(pan_val->value());
	return p;*/
	return NULL;
}

void PanEffect::process_audio(quint8 *samples, int nb_bytes) {
	double pval = pan_val->get_double_value();
	if (pval != 0) {
        for (int i=0;i<nb_bytes;i+=4) {
            qint16 left_sample = (qint16) (((samples[i+1] & 0xFF) << 8) | (samples[i] & 0xFF));
            qint16 right_sample = (qint16) (((samples[i+3] & 0xFF) << 8) | (samples[i+2] & 0xFF));

			double val = qPow(pval*0.01, 3);
            if (val < 0) {
                // affect right channel
                right_sample *= (1-std::abs(val));
            } else {
                // affect left channel
                left_sample *= (1-val);
            }

            samples[i+3] = (quint8) (right_sample >> 8);
            samples[i+2] = (quint8) right_sample;
            samples[i+1] = (quint8) (left_sample >> 8);
            samples[i] = (quint8) left_sample;
        }
    }
}
