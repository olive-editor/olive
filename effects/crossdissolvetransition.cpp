#include "transition.h"

#include <QOpenGLFunctions>

CrossDissolveTransition::CrossDissolveTransition() : Transition(VIDEO_DISSOLVE_TRANSITION) {}

void CrossDissolveTransition::process_transition(double progress) {
    float color[4];
    glGetFloatv(GL_CURRENT_COLOR, color);
	glColor4f(1.0, 1.0, 1.0, color[3]*progress);
}

void CrossDissolveTransition::process_audio(double range_start, double range_end, quint8* samples, int nb_samples, bool reverse) {
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

Transition* CrossDissolveTransition::copy() {
	Transition* t = new CrossDissolveTransition();
	t->length = length;
	return t;
}
