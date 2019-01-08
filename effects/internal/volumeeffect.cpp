#include "volumeeffect.h"

#include <QGridLayout>
#include <QLabel>
#include <QtMath>
#include <stdint.h>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"

VolumeEffect::VolumeEffect(Clip* c, const EffectMeta *em) : Effect(c, em) {
	EffectRow* volume_row = add_row("Volume");
	volume_val = volume_row->add_field(EFFECT_FIELD_DOUBLE, "volume");
	volume_val->set_double_minimum_value(0);

	// set defaults
	volume_val->set_double_default_value(100);
}

void VolumeEffect::process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int) {
	double interval = (timecode_end-timecode_start)/nb_bytes;
	for (int i=0;i<nb_bytes;i+=4) {
		double vol_val = log_volume(volume_val->get_double_value(timecode_start+(interval*i), true)*0.01);

		qint32 right_samp = (qint16) (((samples[i+3] & 0xFF) << 8) | (samples[i+2] & 0xFF));
		qint32 left_samp = (qint16) (((samples[i+1] & 0xFF) << 8) | (samples[i] & 0xFF));

		left_samp *= vol_val;
		right_samp *= vol_val;

		if (left_samp > INT16_MAX) {
			left_samp = INT16_MAX;
		} else if (left_samp < INT16_MIN) {
			left_samp = INT16_MIN;
		}

		if (right_samp > INT16_MAX) {
			right_samp = INT16_MAX;
		} else if (right_samp < INT16_MIN) {
			right_samp = INT16_MIN;
		}

		samples[i+3] = (quint8) (right_samp >> 8);
		samples[i+2] = (quint8) right_samp;
		samples[i+1] = (quint8) (left_samp >> 8);
		samples[i] = (quint8) left_samp;
	}
}
