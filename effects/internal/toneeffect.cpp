#include "toneeffect.h"

#include <QtMath>

#define TONE_TYPE_SINE 0

#include "project/clip.h"
#include "project/sequence.h"
#include "debug.h"

ToneEffect::ToneEffect(Clip* c, const EffectMeta *em) : Effect(c, em), sinX(INT_MIN) {
    type_val = add_row("Type:")->add_field(EFFECT_FIELD_COMBO, "type");
	type_val->add_combo_item("Sine", TONE_TYPE_SINE);

    freq_val = add_row("Frequency:")->add_field(EFFECT_FIELD_DOUBLE, "frequency");
	freq_val->set_double_minimum_value(20);
	freq_val->set_double_maximum_value(20000);
	freq_val->set_double_default_value(1000);

    amount_val = add_row("Amount:")->add_field(EFFECT_FIELD_DOUBLE, "amount");
	amount_val->set_double_minimum_value(0);
	amount_val->set_double_maximum_value(100);
	amount_val->set_double_default_value(25);

    mix_val = add_row("Mix:")->add_field(EFFECT_FIELD_BOOL, "mix");
    mix_val->set_bool_value(true);
}

void ToneEffect::process_audio(double timecode_start, double timecode_end, quint8 *samples, int nb_bytes, int) {
	double interval = (timecode_end - timecode_start)/nb_bytes;
	for (int i=0;i<nb_bytes;i+=4) {
		double timecode = timecode_start+(interval*i);

		qint16 left_tone_sample = qSin((2*M_PI*sinX*freq_val->get_double_value(timecode, true))/parent_clip->sequence->audio_frequency)*log_volume(amount_val->get_double_value(timecode, true)*0.01)*INT16_MAX;
		qint16 right_tone_sample = left_tone_sample;

		// mix with source audio
		if (mix_val->get_bool_value(timecode, true)) {
			qint16 left_sample = (qint16) (((samples[i+1] & 0xFF) << 8) | (samples[i] & 0xFF));
			qint16 right_sample = (qint16) (((samples[i+3] & 0xFF) << 8) | (samples[i+2] & 0xFF));
			left_tone_sample = mix_audio_sample(left_tone_sample, left_sample);
			right_tone_sample = mix_audio_sample(right_tone_sample, right_sample);
		}

		samples[i+3] = (quint8) (right_tone_sample >> 8);
		samples[i+2] = (quint8) right_tone_sample;
		samples[i+1] = (quint8) (left_tone_sample >> 8);
		samples[i] = (quint8) left_tone_sample;

		int presin = sinX;
		sinX++;
		if (sinX < presin) {
			dout << "[WARNING] Tone effect overflowed";
		}
	}
}
