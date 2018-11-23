#include "linearfadetransition.h"

LinearFadeTransition::LinearFadeTransition(Clip* c, Clip* s, const EffectMeta* em) : Transition(c, s, em) {}

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
