#include "transition.h"

LinearFadeTransition::LinearFadeTransition() : Transition(AUDIO_LINEAR_FADE_TRANSITION) {}

void LinearFadeTransition::process_audio(double range_start, double range_end, quint8* samples, int nb_samples, bool reverse) {
	double interval = (range_end - range_start) / nb_samples;

	for (int i=0;i<nb_samples;i+=2) {
		qint16 samp = (qint16) (((samples[i+1] & 0xFF) << 8) | (samples[i] & 0xFF));

		if (reverse) {
			samp *= 1 - (range_start + (interval * i));
		} else {
			samp *= range_start + (interval * i);
		}

		samples[i+1] = (quint8) (samp >> 8);
		samples[i] = (quint8) samp;
	}
}

Transition* LinearFadeTransition::copy() {
	Transition* t = new LinearFadeTransition();
	t->length = length;
	return t;
}
