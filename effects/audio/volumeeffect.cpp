#include "volumeeffect.h"

#include <QGridLayout>
#include <QLabel>
#include <QtMath>
#include <QDebug>
#include <stdint.h>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"

VolumeEffect::VolumeEffect(Clip* c) : Effect(c, EFFECT_TYPE_AUDIO, AUDIO_VOLUME_EFFECT) {
	EffectRow* volume_row = add_row("Volume:");
	volume_val = volume_row->add_field(EFFECT_FIELD_DOUBLE);
	volume_val->set_double_minimum_value(0);
	volume_val->set_double_maximum_value(400);

	// set defaults
	volume_val->set_double_default_value(100);

	connect(volume_val, SIGNAL(changed()), this, SLOT(field_changed()));
}

void VolumeEffect::process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int) {
//	qDebug() << timecode_start << timecode_end;
	double interval = (timecode_end-timecode_start)/nb_bytes;
	for (int i=0;i<nb_bytes;i+=2) {
		double vol_val = qSqrt(volume_val->get_double_value(timecode_start+(interval*i), true)*0.01);
		qint32 samp = (qint16) (((samples[i+1] & 0xFF) << 8) | (samples[i] & 0xFF));
		samp *= vol_val;
		if (samp > INT16_MAX) {
			samp = INT16_MAX;
		} else if (samp < INT16_MIN) {
			samp = INT16_MIN;
		}

		samples[i+1] = (quint8) (samp >> 8);
		samples[i] = (quint8) samp;
	}
}
